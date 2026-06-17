# Project Context: ESP32 Sumobot Firmware

## Business Domain
Hobby combat robotics — **sumobot** class. The robot competes in a ring against another robot; the goal is to push the opponent out of the ring. This project covers **firmware/software only**; the mechanical and electrical builds are predetermined (see Constraints).

## Current Situation
The hardware design is fixed and parts have been selected. There is no existing firmware. The operator currently has no way to drive the robot. The firmware must turn the bill of materials below into a controllable, safe, competition-capable push-only sumobot driven by an Xbox controller over Bluetooth LE.

## Stakeholders
| Role | Name/Group | Interest |
|------|------------|----------|
| Operator / Pilot | Robot owner | Reliable, low-latency, intuitive control during matches |
| Builder / Maintainer | Robot owner | Easy to flash, tune, and diagnose without code changes |
| Opponent / Bystander | Other pilots, spectators | Predictable safe-stop behavior; no runaway robot |

## Scope

### In Scope (firmware)
- Bluetooth LE pairing/reconnect with an Xbox Series controller
- Stick/trigger → tank-drive motor mixing
- PWM + direction control for two BTS7960 H-bridge drivers (one per side, two motors in parallel per driver)
- Arm/disarm state machine with operator-initiated emergency stop
- Connection-loss failsafe (motors stop)
- Status feedback via on-board LED (and optionally serial log)
- Tunable parameters (deadband, max power, slew rate) stored on-device (NVS)
- Telemetry over serial (battery, controller link, fault flags)

### Out of Scope
- Mechanical or electrical design (parts are fixed — see Constraints)
- Autonomous behaviors, opponent detection, edge/line detection
- Weapon control (the robot is push-only — no servos, flippers, or spinners)
- Match scoring, timing, or referee logic
- Multi-robot coordination, OTA updates, mobile app, cloud connectivity
- Battery management circuitry (the LiPo's protection is external)

## Constraints

### Hardware (fixed bill of materials)
| Component | Qty | Notes |
|-----------|----:|-------|
| Pololu 37D 30:1 metal gearmotor (12 V, helical pinion) | 4 | Two per side, mechanically coupled to one wheel each |
| Pololu stamped aluminum L-bracket for 37D motors | 4 | Mounting hardware |
| BTS7960 H-bridge motor driver | 2 | One per side; each drives 2 motors wired in parallel |
| ESP-WROOM-32 dev board (WiFi + BLE, dual-core) | 1 | Main MCU |
| SUNPADOW 14.8 V 4S LiPo, 25 C, 2250 mAh, XT60 | 1 | Motor + logic power source |
| LM2596 buck converter | 1 | Steps 14.8 V down to 5 V for ESP32 |
| FingerTech 2.5" Shore A20 urethane sumo wheel | 4 | Fixed (no steering) — tank drive only |

### Mechanical / Drive
- **4 motors, fixed wheels, tank drive.** Steering by spinning each side in opposite directions (skid steer / differential drive).
- **Push-only.** No active weapon. The whole robot body is the pushing surface.
- Two motors per side are driven as one logical unit (same direction, same speed) by a single BTS7960.

### Robot Class
- **~4 kg robot, ~40 × 40 × 40 cm envelope.** No formal competition ruleset compliance required (hobby driving). No mandatory start delay, no required edge/line detection.

### Software / Toolchain
- Target MCU: ESP32 (ESP-WROOM-32 module)
- Input: Xbox Series controller over **Bluetooth LE** (native ESP32 BLE)
- No safety-of-life requirements, but motors are high-current and the robot is heavy enough to cause damage — fail-stop on disconnect is mandatory

## Assumptions
- The two motors on the same side are wired in parallel into a single BTS7960 channel and behave as a single actuator.
- The BTS7960 is driven with two PWM signals (one per rotation direction) plus enable pins, controlled from ESP32 GPIO/LEDC.
- The Xbox controller exposes the standard HID gamepad mapping over BLE that the ESP32 BLE-HID stack can parse.
- Battery voltage is sensed via a resistor divider into an ADC pin (to be defined in design).
- Tuning will be done on a bench, not mid-match.

## Resolved Decisions
- **BLE-HID library:** **Bluepad32** — mature library for ESP32-as-host receiving from Xbox Series controllers.
- **BTS7960 current sensing (IS pins):** **Deferred to v2** — brownout failsafe covers the most common failure path; revisit once the bot is driving.
- **LED status vocabulary:** **Monochrome blink patterns on the onboard GPIO LED** (no RGB LED in BOM).
- **Telemetry transport:** **USB-serial only for v1** — bench tuning over USB; BLE/UART broadcast deferred.

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
