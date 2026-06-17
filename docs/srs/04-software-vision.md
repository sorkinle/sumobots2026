# Step 4: Software Vision (SV)

> **Purpose:** Single-page-ish elevator pitch for the ESP32 sumobot firmware. Distills the positioning, stakeholders, major features, architectural shape, and bounded scope into a form that a new contributor or stakeholder can read in five minutes and orient themselves.

## Positioning Statement

**For** hobby combat-robotics builders **who** want to drive a custom 4-motor tank-drive sumobot from an Xbox controller without writing firmware from scratch,
**the** ESP32 Sumobot Firmware **is a** purpose-built embedded control program
**that** turns a fixed bill of materials (ESP-WROOM-32 + two BTS7960 drivers + four Pololu 37D gearmotors + 4S LiPo) into a safe, low-latency, tunable, push-only sumobot driven over Bluetooth-LE.
**Unlike** a generic RC-car sketch or a hand-rolled motor driver loop, **our firmware** treats safety (arm/disarm + link-loss + brownout failsafe) as a first-class concern, exposes runtime tuning over USB-serial, and persists configuration in NVS so the operator can iterate on "feel" without re-flashing.

## Goals (Quantified)

| # | Goal | Target |
|---|------|--------|
| G1 | End-to-end input → motor-output latency | ≤ 50 ms (controller HID report received → PWM duty updated) |
| G2 | Link-loss → motors-off time | ≤ 250 ms after last received HID report |
| G3 | Automatic BLE reconnect after dropout | ≤ 5 s under normal RF conditions |
| G4 | Brownout cut-off | Vbatt < configurable threshold (default ≈ 13.2 V for 4S, i.e. 3.3 V/cell) |
| G5 | Tuning round-trip (change value → effect on bot) | No reflash; serial command, persist to NVS, immediate effect |
| G6 | Time-to-arm from cold boot (battery plug-in to ARMED-ready) | ≤ 15 s including BLE connection |

## Stakeholders

| Stakeholder | Primary Interest | Key CPs |
|-------------|------------------|---------|
| **Operator / Pilot** | Responsive, predictable, intuitive control during a match | CP.1, CP.2, CP.6, CP.7 |
| **Builder / Maintainer** | Easy to flash once, then tune by serial; clear fault diagnosis | CP.8, CP.9 |
| **Opponent / Bystander** | Robot never moves when it shouldn't; stops when it should | CP.3, CP.4, CP.5 |
| **Robot itself (hardware investment)** | Survives stalls, brownouts, accidental disarm-time inputs | CP.5 |

## Prioritized Feature List

> Priority follows MoSCoW. "Must" features are required for a competition-ready first release; "Should" features are highly desirable; "Could" are nice-to-haves; "Won't" is explicitly out for this version.

### Must Have (MVP)
| # | Feature | Source |
|---|---------|--------|
| F1 | Bluetooth-LE pairing and auto-reconnect with an Xbox Series controller | CN.5 |
| F2 | Stick/trigger → per-side wheel command (tank or arcade mapping) | CN.8, CN.9 |
| F3 | PWM + direction output for two BTS7960 drivers (left side + right side) | CN.1, CN.2 |
| F4 | ARM / DISARM state machine gated by explicit operator action | CN.7 |
| F5 | Emergency-stop on dedicated controller button | CN.3 |
| F6 | Link-loss failsafe (motors-off on heartbeat timeout) | CN.4 |
| F7 | Brownout failsafe (motors-off + FAULT mode on low Vbatt) | CN.6 |
| F8 | Status LED with distinct patterns per mode | CN.10 |

### Should Have
| # | Feature | Source |
|---|---------|--------|
| F9 | USB-serial console with `get` / `set` / `save` / `load` / `dump` commands for tunable parameters | CN.12 |
| F10 | Persist tunable parameters in ESP32 NVS across reboots | CN.13 |
| F11 | Periodic USB-serial telemetry stream (configurable rate) | CN.14 |
| F12 | On-demand state snapshot over USB-serial | CN.11 |

### Could Have
| # | Feature | Source |
|---|---------|--------|
| F13 | Configurable acceleration slew (rate-limit on per-side output changes) | CP.5, CP.6 (tuning) |
| F14 | Configurable response curve (linear / expo) | CN.9 |
| F15 | LED brightness or RGB color codes (instead of monochrome blink patterns) | CP.7 |
| F16 | Driver thermal / over-current protection via BTS7960 IS pin | CP.5 (open question in 00-context.md) |
| F17 | Telemetry also broadcast over BLE or secondary UART | CP.9 (open question in 00-context.md) |

### Won't Have (this release)
| # | Feature | Why |
|---|---------|-----|
| W1 | Autonomous attack / opponent detection | Push-only manual control, per scope |
| W2 | Edge / line / ring-out detection | Not required by the chosen "no ruleset" class |
| W3 | OTA firmware update | Out of scope; USB-flash is sufficient |
| W4 | Companion mobile app | USB-serial console is sufficient for tuning |
| W5 | Multi-robot or cloud features | Out of scope |
| W6 | Weapon control (servo / spinner / flipper) | Robot is push-only by design |

## Architecture Overview

The firmware is organized into **8 single-responsibility components**, communicating through narrow internal interfaces, with a **strict safety invariant**: operator input reaches motor outputs **only** through the State Machine's `ARMED` gate; the Failsafe can force motors-off without input-pipeline cooperation.

```
Xbox  ──BLE──►  ① BLE Controller  ─►  ② Drive Mixer  ─►  ③ Motor Output Driver  ─► BTS7960 ×2 ─► Motors
                                              ▲                    ▲
                                              │                    │ gate (ARMED?)
                            ⑥ Config Store ──┘           ④ State Machine ◄─── ⑤ Failsafe ◄── Vbatt
                                                                ▲
                                                                │
                                                       ⑦ Status LED   ⑧ Serial Console / Telemetry ↔ PC
```

(Full block diagram and component table live in `02-software-glance.md`.)

**Key architectural decisions baked in:**
1. **Single ARMED gate** — there is exactly one code path from input to motors, and it passes through one boolean.
2. **Failsafe is asynchronous** — it does not wait for the input loop; it can force the State Machine into FAULT from any context.
3. **Config Store is the only writer of NVS** — no other component persists state, simplifying recovery after corruption.
4. **Serial Console is read-mostly w.r.t. motor control** — it can change tuning parameters but cannot directly command motors. (This bounds the trust required in the serial interface.)
5. **One Motor Output Driver instance owns all motor GPIOs** — no other component touches PWM, direction, or enable pins.

## Target Environment

| Layer | Choice / Constraint |
|-------|--------------------|
| MCU | ESP32 (ESP-WROOM-32 module) — dual-core Xtensa LX6, ~240 MHz |
| Toolchain | Arduino-ESP32 *or* ESP-IDF (open — see 00-context.md) |
| BLE-HID library | **Bluepad32** (decided) |
| PWM | ESP32 LEDC peripheral, dedicated channel per direction pin per side |
| ADC | One ESP32 ADC pin via resistor divider for Vbatt |
| NVS | ESP32 built-in NVS partition |
| Console | USB-serial via UART0 at a fixed baud (e.g., 115200) |
| Status LED | Onboard GPIO LED (single color) — RGB is a "Could Have" |

## Scope Boundary (Reminder)

**In scope:** firmware behavior on the ESP32 — pairing, mixing, motor output, state, failsafe, tuning console, telemetry.
**Out of scope:** all hardware decisions, all mechanical design, all autonomous behavior, all weapon control, all match-rules logic.

(Full scope statement lives in `00-context.md`.)

## Resolved Decisions (before Step 5)

1. **BLE-HID library:** Bluepad32
2. **BTS7960 IS-pin current sensing:** Deferred to v2 (brownout failsafe covers the dominant failure mode for v1)
3. **LED pattern vocabulary:** Monochrome blink patterns on onboard GPIO LED
4. **Telemetry transport:** USB-serial only for v1

## Quality Gate (Step 4)

- [x] Positioning statement clear and follows the "For / who / the / is / that / unlike" form
- [x] Quantified goals (G1–G6) tied to CPs/CNs
- [x] All stakeholders identified and linked to CPs
- [x] Major features listed and prioritized (MoSCoW)
- [x] Every "Must" / "Should" / "Could" feature traces to a CN or CP
- [x] "Won't Have" list explicitly captured
- [x] Architecture overview consistent with `02-software-glance.md`
- [x] File saved: `04-software-vision.md`

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
