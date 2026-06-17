# Sumobot Firmware — Design / Implementation Notes

> **Companion to:** `docs/srs/` (Problem-Based SRS for ESP32 Sumobot Firmware)
> **Audience:** Implementer (you / future you)
> **Purpose:** Turn the WHAT in the SRS into a concrete HOW — toolchain, wiring, code layout, library choices, protocols, and bring-up procedure.
> **Trace policy:** Every section that constrains behavior cites the FR/NFR it implements, so a reviewer can audit "does this design satisfy the spec?"

---

## 1. Hardware Recap

From `docs/srs/00-context.md` (fixed):

| # | Item | Quantity |
|---|------|---------:|
| 1 | Pololu 37D metal-gear motor with encoder (encoder ignored by firmware) | 4 |
| 2 | BTS7960 43 A dual H-bridge driver | 2 |
| 3 | ESP-WROOM-32 dev board | 1 |
| 4 | Zeee 4 S 14.8 V 2 250 mAh LiPo | 1 |
| 5 | LM2596 buck (5 V rail for ESP32 + LED + driver logic) | 1 |
| 6 | FingerTech 2.5″ Shore A20 wheel | 4 |

**Wiring topology**

```
                   [4S LiPo 14.8V]
                  ┌──────┬───────┐
                  │      │       │
                  ▼      ▼       ▼
        BTS7960-L  BTS7960-R   LM2596 → 5V
        (drives    (drives      ├─ ESP-WROOM-32 (VIN/USB Vbus)
         L1, L2)    R1, R2)     ├─ Status LED (via current-limit R)
                                └─ BTS7960 VCC (logic)
              All grounds tied to common GND
        Vbatt sense: divider 22k / 4.7k from V+ to GND → ADC pin
```

The two motors on each side wire to one BTS7960's M+/M− outputs in parallel — they are commanded as one logical actuator per side ("Left" / "Right"). This is what the SRS calls *Motor Output* per side.

---

## 2. Pin Map (GPIO Assignment)

ESP-WROOM-32 (DevKitC) pinout. All PWM is via ESP32 LEDC peripheral (hardware PWM, no jitter from main loop).

| Function | GPIO | Direction | Peripheral / Notes |
|---|---:|---|---|
| **Left  – RPWM** (forward duty) | 25 | Output, PWM | LEDC ch 0, 20 kHz, 10-bit |
| **Left  – LPWM** (reverse duty) | 26 | Output, PWM | LEDC ch 1, 20 kHz, 10-bit |
| **Left  – EN** (R_EN + L_EN tied) | 27 | Output, digital | HIGH when ARMED, else LOW |
| **Right – RPWM** | 32 | Output, PWM | LEDC ch 2, 20 kHz, 10-bit |
| **Right – LPWM** | 33 | Output, PWM | LEDC ch 3, 20 kHz, 10-bit |
| **Right – EN** (R_EN + L_EN tied) | 14 | Output, digital | HIGH when ARMED, else LOW |
| **Vbatt sense** | 34 | Input only, ADC1_CH6 | 22k / 4.7k divider, 11 dB atten |
| **Status LED** | 2 | Output, digital | Onboard blue LED (DevKitC) |
| **USB-serial** | – | – | UART0 via the dev board's USB-UART bridge |

**Pin rationale**

- All PWM/EN pins are output-capable GPIOs and avoid the strapping pins 0, 2, 12, 15 (except GPIO 2 used as LED only, which is fine — it's only an issue if held LOW at boot, and our default state is OFF/LOW after reset).
- GPIO 34 is input-only and on ADC1, so it doesn't conflict with Wi-Fi or BLE (which monopolizes ADC2).
- Tying R_EN and L_EN of each BTS7960 together is a deliberate simplification: the SRS only requires a single enable per side (one logical actuator).

**Vbatt divider math** (22 kΩ top, 4.7 kΩ bottom)

```
V_adc = V_batt × 4.7 / (22 + 4.7) = V_batt × 0.1760
V_batt(max, full charge 4S) = 16.8 V → V_adc ≈ 2.96 V   ✅ within 3.3 V
V_batt(default cutoff 13.2 V)        → V_adc ≈ 2.32 V
V_batt(empty 3.0 V/cell)             → V_adc ≈ 2.11 V
```

ADC1 with 11 dB attenuation has a useful range of ~150 mV to ~2.45 V (some calibration drift past that; consult the ESP-IDF ADC calibration appnote). Our window 2.1–3.0 V falls in the upper-saturation region — use **eFuse-based calibration** (`adc1_get_raw` + `esp_adc_cal_raw_to_voltage`) and verify against a multimeter at bring-up. If readings saturate, swap to **6 dB attenuation** and adjust the divider to e.g. 33 kΩ / 4.7 kΩ so V_adc(16.8 V) ≈ 2.10 V.

---

## 3. Toolchain & Project Layout

**Toolchain:** **Arduino IDE 2.x** with the ESP32 core. This is the toolchain the project uses.

**Arduino IDE setup**

1. Install the **ESP32 boards** (Boards Manager → "esp32" by Espressif).
2. Install **Bluepad32 for Arduino** — follow its "Arduino IDE" guide, which adds a Bluepad32-enabled ESP32
   board variant plus the `Bluepad32.h` library. Select that board so the ESP32 acts as a BLE host.
3. Board: **ESP32 Dev Module**; Upload Speed 921600; Serial Monitor 115200 baud.

Full step-by-step setup (board-manager URLs, the "ESP32 + Bluepad32" board-group gotcha, flashing, pairing,
and troubleshooting) lives in `../../firmware/ARDUINO_SETUP.md`.

Each program is one sketch folder (`firmware/<name>/<name>.ino`). Keep the hardware-independent drive logic in
plain `.h`/`.cpp` files beside the `.ino` so the same code can be unit-tested on a PC (see section 8).

(PlatformIO can also build this, but Arduino IDE is the supported path.)

**Source tree**

```
firmware/
├── src/
│   ├── main.cpp / .ino          # setup() / loop() — minimal, just runs scheduler
│   ├── input/
│   │   ├── ble_hid.cpp/.h       # Bluepad32 glue; produces ControllerSnapshot
│   │   └── controller_map.h     # Xbox button → semantic name
│   ├── control/
│   │   ├── mixer.cpp/.h         # arcade/tank mapping, deadband, curve, slew
│   │   └── state_machine.cpp/.h # BOOT / PAIRING / DISARMED / ARMED / FAULT
│   ├── output/
│   │   ├── motor_pair.cpp/.h    # BTS7960 driver, ARMED-gate guard
│   │   └── status_led.cpp/.h    # LED blink patterns per mode
│   ├── safety/
│   │   └── failsafe.cpp/.h      # FR-006 link-loss + FR-009 brownout (one Vbatt IIR)
│   ├── config/
│   │   ├── params.cpp/.h        # Tunable struct + defaults + NVS load/save
│   │   └── nvs_keys.h           # Single source of truth for NVS keys
│   ├── console/
│   │   └── serial_cli.cpp/.h    # FR-016 line parser + command dispatch
│   ├── telemetry/
│   │   └── telemetry.cpp/.h     # FR-015 snapshot + FR-018 stream
│   └── util/
│       └── time_us.h            # esp_timer_get_time() helpers
└── test/                        # host (PC) unit tests for mixer/state (see section 8)
```

This layout maps 1-for-1 to the 8 firmware components in `02-software-glance.md`. In the Arduino IDE the
sketch is a folder containing the `.ino` plus these files (kept beside it, or under a `src/` subfolder); host
tests live in `firmware/test/`.

---

## 4. Software Architecture

**Concurrency model:** Cooperative, single-loop, 1 kHz tick driven by `esp_timer`. BLE-HID runs on its own FreeRTOS task supplied by Bluepad32, which posts events into a queue we drain at the top of every loop. This is simpler than a multi-task model and makes the latency budget (NFR-001) trivial to reason about: every external event is acted on within one tick of arrival.

**Main loop (10 ms tick → 100 Hz, satisfies FR-001):**

```
every 10 ms:
    Drain BLE-HID event queue → ControllerSnapshot   (carries last_event_ms)
    vbatt_filt = iir(vbatt_filt, read_vbatt_raw())   // single Vbatt IIR, one site
    faults = Failsafe.tick(controller, vbatt_filt, params)
    state = StateMachine.tick(state, controller, faults, params)
    if state == ARMED:
        (left_cmd, right_cmd) = Mixer.compute(controller, params)
    else:
        (left_cmd, right_cmd) = (0, 0)
    MotorPair.apply(left_cmd, right_cmd)    // ARMED-gate enforced inside
    StatusLED.tick(state)
    SerialCli.poll_nonblocking()
    Telemetry.tick(state, controller, motors, vbatt_filt)
```

**Bluepad32 callback** runs on a separate task and just enqueues `(timestamp_us, controller_state)`; the main loop drains it. This is what keeps end-to-end latency bounded by one 10 ms tick + Bluepad32's own forward-path delay.

**Latency budget (NFR-001):** With a 10 ms tick the worst-case event-to-PWM latency is ~10 ms loop + Bluepad32 stack delay (typ. 12–20 ms over BLE-HID) ≈ **20–30 ms typical, ≤ 50 ms p95** — well inside budget.

---

## 5. Subsystem Detail

### 5.1 Input — BLE-HID via Bluepad32 (FR-007, FR-008)

```cpp
// input/ble_hid.h
struct ControllerSnapshot {
    int16_t lx, ly, rx, ry;     // -512..+511 (Bluepad32 native)
    int16_t lt, rt;              // 0..1023
    uint16_t buttons;            // bitfield: A=1, B=2, X=4, Y=8, LB=16, RB=32, MENU=64
    uint32_t last_event_ms;      // millis() at receipt
    bool valid;                  // false until first event after pairing
};

void ble_hid_init();                       // boots Bluepad32, restores paired controller
ControllerSnapshot ble_hid_latest();
void ble_hid_forget_all();                 // FR-007 helper: clears paired controllers
```

Initial pairing (FR-007): on boot, if NVS has no remembered controller, enter **PAIRING** mode and call `BP32.enableNewBluetoothConnections(true)`. On first successful HID report, write the controller's BD address to NVS, then transition to DISARMED. From then on, only the remembered controller is accepted.

Auto-reconnect (FR-008): default Bluepad32 behavior after `enableNewBluetoothConnections(false)`. The BLE stack handles the retry loop; we measure success via NFR-002.

### 5.2 Mixer — Drive Math (FR-003, FR-011, FR-012, FR-013)

```cpp
// control/mixer.h
struct DriveParams {
    uint8_t mapping_mode;     // 0=arcade, 1=tank
    uint16_t deadband;        // raw counts on stick axis, 0..200
    uint16_t max_power;       // 0..1023 (10-bit PWM)
    uint16_t slew;            // max delta in PWM counts per 10ms tick
    uint8_t curve_type;       // 0=linear, 1=expo, 2=power-2
    uint8_t curve_factor;     // 0..100 (% curvature for expo/power)
};

struct DriveCommand {
    int16_t left;             // signed PWM, range -max_power..+max_power
    int16_t right;
};

DriveCommand mixer_compute(const ControllerSnapshot& s, const DriveParams& p);
```

**Arcade mapping** (default):

```
forward = LY  (or RT-LT for trigger-style; mapping_mode picks)
steer   = RX
left  = forward + steer
right = forward - steer
clamp to [-max_power, +max_power]
```

**Tank mapping:** `left = LY`, `right = RY` directly.

Deadband: any axis with `|raw| < deadband` is treated as 0. Curves are applied **after** deadband and **before** slew. Slew is per-tick: `out = clamp(prev + slew_sign × min(|target − prev|, slew), …)`.

### 5.3 Output — BTS7960 Motor Pair (FR-010, NFR-003)

```cpp
// output/motor_pair.h
class MotorPair {
public:
    void begin(int rpwm, int lpwm, int en, uint8_t ledc_ch_r, uint8_t ledc_ch_l);

    // The ONLY public method that writes PWM. The ARMED-gate guard lives here
    // (NFR-003). Callers always invoke this — even when state != ARMED — so the
    // guard sees every call.
    void apply(int16_t signed_pwm, bool armed);
private:
    int rpwm_, lpwm_, en_;
    uint8_t ch_r_, ch_l_;
};

// motor_pair.cpp:
void MotorPair::apply(int16_t cmd, bool armed) {
    if (!armed) {                    // ← single ARMED-gate guard
        ledcWrite(ch_r_, 0);
        ledcWrite(ch_l_, 0);
        digitalWrite(en_, LOW);
        return;
    }
    digitalWrite(en_, HIGH);
    if (cmd >= 0) {
        ledcWrite(ch_l_, 0);
        ledcWrite(ch_r_, cmd);
    } else {
        ledcWrite(ch_r_, 0);
        ledcWrite(ch_l_, -cmd);
    }
}
```

**Why this satisfies NFR-003:** there is exactly one code site that writes PWM (`ledcWrite` in `MotorPair::apply`), and the `!armed` early-return guards it. A grep for `ledcWrite(ch_r_|ch_l_` returns one method. Mutating the guard fails the state-injection unit test (`test/test_armed_gate.cpp`).

**LEDC config:** `ledcSetup(channel, 20000, 10);` then `ledcAttachPin(gpio, channel);` in `begin()`. 20 kHz is above the audible band; 10-bit (0..1023) gives a workable resolution.

### 5.4 State Machine (FR-004, FR-005, FR-006, FR-009, FR-010, FR-019)

```cpp
// control/state_machine.h
#include "safety/failsafe.h"   // Faults (the failsafe signal)

enum class Mode : uint8_t { BOOT, PAIRING, DISARMED, ARMED, FAULT };

Mode state_machine_tick(Mode current,
                        const ControllerSnapshot& c,
                        Faults f,
                        const Params& p);
```

**Transition table** (the authoritative source-of-truth):

| From | Event / Condition | To | FR |
|---|---|---|---|
| BOOT | NVS has no paired controller | PAIRING | FR-007 |
| BOOT | NVS has paired controller | DISARMED | FR-007 |
| PAIRING | First valid HID event from new controller | DISARMED | FR-007 |
| DISARMED | LB+RB+A held continuously ≥ `arm_hold_ms` (combo fixed in code) | ARMED | FR-005 |
| ARMED | B pressed (rising edge) | DISARMED | FR-004 |
| ARMED | `f.linkloss` (Failsafe: no HID event for `linkloss_timeout_ms`) | DISARMED | FR-006 |
| ARMED | `f.brownout` (Failsafe: filtered Vbatt < threshold) | FAULT | FR-009 |
| FAULT | MENU held ≥ 2 s **AND** `!f.brownout` | DISARMED | FR-019 |

**Invariants:**

- `MotorPair::apply(cmd, state == Mode::ARMED)` is the **only** way to call into motor output → FR-010 + NFR-003 are structurally guaranteed.
- The FAULT state ignores stick/button input for motor control; it accepts only the MENU-hold fault-clear gesture.

### 5.5 Failsafe / Watchdog (FR-006, FR-009)

One unit (SRS component ⑤) owns both safety monitors and produces the `Faults` signal the State Machine consumes. It holds **no** Vbatt filter of its own — the main loop maintains the single IIR (`vbatt_filt`) and passes it in — so `failsafe_tick` is a pure function of (latest input, filtered Vbatt, params), with nothing to construct or initialize.

```cpp
// safety/failsafe.h
struct Faults {
    bool linkloss;   // no HID event for linkloss_timeout_ms
    bool brownout;   // filtered Vbatt under threshold
};

Faults failsafe_tick(const ControllerSnapshot& c, float vbatt_filt, const Params& p);
```

```cpp
// safety/failsafe.cpp
Faults failsafe_tick(const ControllerSnapshot& c, float vbatt_filt, const Params& p) {
    Faults f;
    f.linkloss = (millis() - c.last_event_ms) >= p.linkloss_timeout_ms;
    f.brownout = vbatt_filt < p.vbatt_threshold;
    return f;
}
```

- **Link-loss** default timeout **250 ms** (covers 2 missed BLE intervals of typ. 30–50 ms with margin).
- **Brownout** reads the loop's single-pole IIR `vbatt_filt` (α = `tick_ms / (tick_ms + filter_ms)`). It latches in the State Machine; the FR-019 fault-clear gesture is the only exit, and only once `vbatt_filt > threshold`.

### 5.6 Status LED (FR-014, NFR-005)

Blink patterns, generated by a 100 ms phase counter:

| Mode | Pattern (50 ms slots, 0 = off, 1 = on) | Cycle |
|---|---|---|
| BOOT | 1 0 0 0 0 0 0 0 | 400 ms |
| PAIRING | 1 0 1 0 0 0 0 0 (rapid double-blink) | 400 ms |
| DISARMED | 1 1 1 1 0 0 0 0 (slow heartbeat) | 800 ms |
| ARMED | 1 1 1 1 1 1 1 1 (solid) | — |
| FAULT | 1 0 1 0 1 0 0 0 (SOS-like rapid triple) | 400 ms |

All five are distinguishable at 1 m within 1 s of a transition (NFR-005).

### 5.7 Config Store — NVS (FR-016, FR-017, NFR-004)

```cpp
// config/params.h
struct Params {
    uint8_t  mapping_mode;
    uint16_t deadband;
    uint16_t max_power;
    uint16_t slew;
    uint8_t  curve_type;
    uint8_t  curve_factor;
    float    vbatt_threshold;
    uint16_t vbatt_filter_ms;
    uint16_t linkloss_timeout_ms;
    uint16_t arm_hold_ms;
    uint16_t telemetry_rate_hz;  // 0 = off
    // pairing state
    uint64_t paired_bd_addr;     // 0 = unpaired
};

void   params_load_defaults(Params& p);
bool   params_load_from_nvs(Params& p);
bool   params_save_to_nvs(const Params& p);
bool   params_set_by_name(Params& p, const char* key, const char* val_str);
```

**Defaults:**

| Key | Default | Range |
|---|---:|---|
| mapping_mode | 0 (arcade) | 0 / 1 |
| deadband | 40 | 0–200 |
| max_power | 700 | 0–1023 |
| slew | 80 | 1–1023 |
| curve_type | 1 (expo) | 0–2 |
| curve_factor | 50 | 0–100 |
| vbatt_threshold | 13.2 V | 11.0–16.8 |
| vbatt_filter_ms | 500 | 50–5000 |
| linkloss_timeout_ms | 250 | 50–2000 |
| arm_hold_ms | 1000 | 200–5000 |
| telemetry_rate_hz | 0 | 0–50 |

NVS namespace: `sumobot`. Keys mirror struct field names. Use `Preferences` library or raw `nvs_flash` — `Preferences` is simpler and built into Arduino-ESP32. The arm-button combo (LB+RB+A) is fixed in code; only `arm_hold_ms` is tunable.

### 5.8 Serial Console (FR-016)

**Grammar:** `<verb> [<key>] [<value>]`, newline-terminated. Whitespace-separated.

| Verb | Form | Effect |
|---|---|---|
| `get` | `get <key>` | print current value |
| `set` | `set <key> <value>` | validate, apply in-RAM (immediate effect) |
| `dump` | `dump` | print full Params struct as `key=value` lines |
| `save` | `save` | flush RAM Params to NVS |
| `load` | `load` | reload Params from NVS (revert RAM changes) |
| `reset` | `reset defaults` | restore compiled-in defaults (RAM only; `save` to persist) |
| `forget` | `forget controller` | clear paired-controller BD address from NVS |
| `pair` | `pair` | force PAIRING mode (only valid in DISARMED) |
| `state` | `state` | print current Mode + faults + last HID timestamp |
| `snap` | `snap` | one-shot status snapshot (FR-015) |
| `tele` | `tele <hz>` | shortcut for `set telemetry_rate_hz <hz>` |
| `help` | `help` | command list |

Implementation: a `command_table[]` of `{verb, min_args, max_args, handler}` is the dispatcher. Unknown verbs print `ERR unknown verb`. Validation errors print `ERR <reason>`. Successes print `OK` (or the requested data).

### 5.9 Telemetry (FR-015, FR-018)

One canonical line format, emitted on `snap` (FR-015) and at `telemetry_rate_hz` (FR-018). Single line, ASCII, KV-separated, easy to grep/script:

```
TELE t=12345678 st=ARMED vb=14.72 lx=0 ly=412 rx=-17 ry=0 btn=0x0020 lL=-280 lR=-280 fault=0
```

Field reference:

| Field | Meaning |
|---|---|
| t | millis() |
| st | state (BOOT/PAIRING/DISARMED/ARMED/FAULT) |
| vb | filtered Vbatt in volts (2 dp) |
| lx,ly,rx,ry | stick axes (raw -512..+511) |
| btn | controller buttons bitfield (hex) |
| lL,lR | last commanded left/right PWM (signed) |
| fault | bitfield: bit0 linkloss, bit1 brownout |

---

## 6. End-to-End Walkthrough (Happy Path)

1. Power on. ESP-WROOM-32 boots, `setup()`:
   - Initializes Serial @ 115200, prints banner.
   - Loads `Params` from NVS (or defaults if first boot).
   - Initializes `MotorPair`s and `StatusLED`. (Failsafe is a stateless function — nothing to construct.)
   - Initializes Bluepad32.
   - If `paired_bd_addr == 0` → State = **PAIRING**; else State = **DISARMED**.
2. Operator turns on Xbox controller. Bluepad32 emits the first HID event. The pairing routine writes the BD address to NVS, transitions to **DISARMED**.
3. State LED switches from rapid-double-blink to slow heartbeat. Wheels remain off.
4. Operator presses and holds **LB + RB + A** for 1 s. State Machine transitions to **ARMED**. LED solid. Wheels now respond to sticks.
5. Operator drives. Each HID event ≤ 50 ms later is reflected at the wheels (NFR-001).
6. Operator presses **B**. State → DISARMED instantly. Wheels off. LED heartbeat.
7. (Failsafe path) If the controller dies / goes out of range while ARMED: 250 ms after the last event, the watchdog forces DISARMED. Wheels off, LED heartbeat. Reconnection (NFR-002) restores DISARMED — does *not* auto-rearm. Operator must arm again.

---

## 7. Bring-Up Procedure

Do these in order on the bench. Each step gives a binary "go / no-go" before adding the next risk.

| # | Step | Pass criterion |
|---|---|---|
| 1 | Flash a blink-only sketch using the agreed pin map | Onboard LED on GPIO 2 blinks |
| 2 | Flash a sketch reading the Vbatt divider and printing volts every 100 ms | Reported voltage matches DMM ± 0.2 V across 12–16 V |
| 3 | Disconnect motors (BTS7960 unpowered). Flash motor-test sketch that sweeps `MotorPair::apply` from −500 to +500 to 0 with `armed=true`. Probe RPWM/LPWM with a scope or LED test rig. | RPWM goes high-duty when cmd>0; LPWM goes high-duty when cmd<0; both low at cmd=0 |
| 4 | Connect motors with **no wheels installed**, robot up on blocks, BTS7960 logic powered, drivers' main rail unpowered. Re-run motor-test. | Confirms wiring is correct without risk |
| 5 | Energize drivers. Repeat motor-test at low duty (max 200). | Motors spin in expected direction; reverse polarity if not |
| 6 | Flash full firmware. Pair controller (first-time pairing flow). | LED PAIRING → DISARMED |
| 7 | Verify arm/disarm gestures and B-button e-stop. | LED + motor behavior matches §6 |
| 8 | Pull controller battery while ARMED. | Wheels stop within ≤ 300 ms; LED returns to DISARMED |
| 9 | Simulate brownout by dialing power supply down. | FAULT entered when filtered Vbatt < threshold for 500 ms; MENU-hold returns to DISARMED once voltage recovers |
| 10 | Tune `deadband`, `curve_factor`, `max_power`, `slew` over serial. | Driving feels right; `save` persists across power-cycle |
| 11 | Install wheels on the ground. | Robot drives. |

---

## 8. Test-to-FR Mapping (How Acceptance Will Be Verified)

| FR | Test |
|---|---|
| FR-001 | Toggle a GPIO inside the loop, scope-measure period — must be ≤ 10 ms |
| FR-002 | `LATENCY_TRACE=1` build, run analyzer — p95 ≤ 50 ms |
| FR-003 | Hold RT fully, observe `lL=lR=max_power` on `snap` |
| FR-004 | While ARMED, press B → LED transitions to DISARMED within ≤ 100 ms; `snap` shows lL=lR=0 |
| FR-005 | Hold LB+RB+A < 1 s → no transition; hold ≥ 1 s → ARMED |
| FR-006 | Pull controller battery → wheels stop; logged `lastevt` shows ≥ 250 ms gap |
| FR-007 | First-boot NVS → enters PAIRING; new controller connects |
| FR-008 | Stress-test: 10× controller power-cycle, ≥ 9 reconnect within 5 s (also satisfies NFR-002) |
| FR-009 | Dial PSU to 12 V → after 500 ms, FAULT |
| FR-010 | State-injection unit test → all non-ARMED states yield PWM=0 (also satisfies NFR-003) |
| FR-011 | `set mapping_mode 1` → tank steering verified by stick test |
| FR-012 | Increase `deadband` → centered-stick wobble disappears |
| FR-013 | `set curve_type 0/1/2` → response curves verified by graphing `snap` outputs while sweeping stick |
| FR-014 | Visual check at 1 m: distinguishable patterns (also satisfies NFR-005) |
| FR-015 | `snap` returns a complete line in < 200 ms |
| FR-016 | Walk through every key in `params.h` (also satisfies NFR-004) |
| FR-017 | `set foo bar` → `save` → power-cycle → `get foo` returns `bar` |
| FR-018 | `tele 20` → 20 lines/s appear on serial |
| FR-019 | Trigger FAULT, then hold MENU ≥ 2 s → DISARMED |

Unit-testable items (mixer, state machine, command parser) are written as **Arduino-free C++** and run on the host (PC) — no board required. The connection-test slice ships such tests in `firmware/test/`; build them with any C++17 compiler, e.g. `g++ -std=c++17 -I ../sumobot_connection_test test_drive_logic.cpp ../sumobot_connection_test/drive_logic.cpp -o t && ./t`. Keeping this logic free of Arduino headers is what makes test-driven development practical.

---

## 9. Open Implementation Choices (Deliberately Deferred)

1. **Current sensing.** BTS7960 has an IS (current-sense) output per half-bridge. The SRS deferred this (00-context.md). When added, route to GPIO 35 and 39 (ADC1), then add a third element to `Faults` and a corresponding FR. No SRS change needed for the deferred plumbing.
2. **OTA updates.** Out of scope for v1. The 1 MB flash budget (NFR-006) leaves room for a 2-app + OTA partition table in a future revision.
3. **IMU / orientation.** Explicitly out of scope.
4. **PWM frequency.** 20 kHz is a starting point. If MOSFET drive at 20 kHz proves wasteful (thermal), drop to 15 kHz (still inaudible to most) and re-measure.

---

## 10. Traceability Quick Index

| Design section | SRS artifacts implemented |
|---|---|
| §2 Pin map | All FRs that touch I/O (FR-002, FR-009, FR-010, FR-014) |
| §4 Main loop | FR-001, FR-002, NFR-001 |
| §5.1 BLE-HID | FR-007, FR-008, NFR-002 |
| §5.2 Mixer | FR-003, FR-011, FR-012, FR-013 |
| §5.3 MotorPair | FR-010, NFR-003 |
| §5.4 State Machine | FR-004, FR-005, FR-006, FR-009, FR-019 |
| §5.5 Failsafe / Watchdog | FR-006, FR-009 |
| §5.6 StatusLED | FR-014, NFR-005 |
| §5.7 Params/NVS | FR-016, FR-017, NFR-004 |
| §5.8 Serial Console | FR-015, FR-016, FR-018 |
| §5.9 Telemetry | FR-015, FR-018 |

Every FR/NFR has a home. Every implementation section justifies its existence by citing an FR/NFR.

---
*Created: 2026-05-20*
*Last Updated: 2026-06-17*
*Author: Project Owner*
