# Sumobot firmware

Arduino-IDE firmware for the ESP32 sumobot. See `../docs/design/implementation-notes.md` for the full design
and `../docs/srs/` for the requirements.

## `sumobot_connection_test/` — Controller → ESP32 → driver → motor

A minimal **connection-test** sketch that proves the end-to-end drive path only. It reads an Xbox controller
over Bluetooth LE (Bluepad32), mixes the sticks into per-side commands, and drives the two ZK-BM1 dual-channel
motor drivers. No arm/disarm state machine, failsafe, telemetry, or NVS yet — those come later.

### Files

| File | Role |
|------|------|
| `sumobot_connection_test.ino` | Device entry: Bluepad32 setup, input→pipeline→motors, ~100 Hz loop |
| `drive_logic.h` / `.cpp` | **Hardware-independent** mixing + ARMED gate + pipeline (unit-tested on PC) |
| `motor_pair.h` / `.cpp` | ZK-BM1 wrapper (LEDC/GPIO); the only code that writes motor hardware |

### Wiring (pin map)

Each ZK-BM1 is a dual-channel driver — one channel per motor (IN1/IN2 and IN3/IN4).

| Side | IN1 (M1 fwd) | IN2 (M1 rev) | IN3 (M2 fwd) | IN4 (M2 rev) |
|------|-------------:|-------------:|-------------:|-------------:|
| Left  |  4 | 18 | 19 | 21 |
| Right | 25 | 26 | 32 | 33 |

Each ZK-BM1 drives two motors on its side (one per channel, same duty). PWM is 20 kHz, 10-bit (0–1023).
No EN pin — the ZK-BM1 does not have one; motors stop when all PWM signals are zero.

**Power:**
- 3S LiPo (11.1 V nominal) → ZK-BM1 VCC (motor power, direct)
- 3S LiPo → LM2596 buck → 5 V → ESP32 VIN
- Common GND bus ties ESP32 GND, buck GND, and ZK-BM1 GND together

### Build & flash (Arduino IDE)

See **[ARDUINO_SETUP.md](ARDUINO_SETUP.md)** for full IDE install, board-package setup, flashing, pairing,
and troubleshooting. In short: install the `esp32` **and** `ESP32 + Bluepad32` board packages, open
`sumobot_connection_test/sumobot_connection_test.ino`, select an **ESP32 + Bluepad32 Arduino** board, and
Upload; then open Serial Monitor at **115200** and pair the controller.

### Driving it — and safety

- Motors run **only while you hold LB + RB together** (the arm gesture for this test). Release either
  shoulder and both sides stop immediately.
- Left stick (Y) = forward/back, right stick (X) = steer (arcade by default).
- A 5-second failsafe timeout stops motors if no controller data is received.
- **Bench-test with the robot up on blocks (wheels off the ground) first.** Confirm direction with the Serial
  Monitor (it prints `armed`, stick values, and per-side duties) before energizing the drivers. If a side
  spins the wrong way, swap that side's motor leads.

## Host tests (test-driven development)

The drive logic is Arduino-free, so it is developed test-first and run on your PC — no board needed:

```sh
cd test
g++ -std=c++17 -I ../sumobot_connection_test test_drive_logic.cpp \
    ../sumobot_connection_test/drive_logic.cpp -o test_drive_logic
./test_drive_logic
```

`clang++` or `python -m ziglang c++` work as drop-in replacements for `g++`. The runner prints
`<n> checks, <n> failed` and exits non-zero on any failure.
