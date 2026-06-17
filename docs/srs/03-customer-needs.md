# Step 3: Customer Needs (CN)

> **Purpose:** Translate the Customer Problems from Step 1 into the **outcomes** the firmware must provide. CNs answer **WHAT** the software must deliver, not HOW.
> **Syntax:** `[Subject] needs [system] to [Verb] [Object] [Condition]`
> **Outcome classes:** Information / Control / Construction / Entertainment

## Needs Specification

| ID | Statement | Outcome Class | Traces To |
|----|-----------|---------------|-----------|
| CN.1 | The operator needs the firmware to translate each controller input into a motor-output update within a single perceptual round-trip, so the robot feels responsive during a match. | Control | CP.1 |
| CN.2 | The operator needs the firmware to deliver the full available per-side motor power when the operator commands maximum throttle, without any firmware-imposed cap below the configured maximum. | Control | CP.2 |
| CN.3 | The operator needs the firmware to drive all motor outputs to zero immediately whenever the operator issues an explicit disarm or emergency-stop command. | Control | CP.3 |
| CN.4 | The firmware needs to drive all motor outputs to zero automatically when the controller link has been silent for longer than a bounded timeout. | Control | CP.3 |
| CN.5 | The operator needs the firmware to establish and re-establish a Bluetooth-LE HID connection with the paired Xbox controller automatically, without requiring a reboot or external action. | Control | CP.4 |
| CN.6 | The firmware needs to drive all motor outputs to zero and enter a fault condition when the measured battery pack voltage falls below a configured low-voltage threshold. | Control | CP.5 |
| CN.7 | The firmware needs to keep motor outputs forced off at all times when the robot is not in the ARMED state, regardless of the current controller input. | Control | CP.5 |
| CN.8 | The operator needs the firmware to provide a selectable input-to-wheel mapping (arcade single-stick or tank dual-stick) so the operator can pick the scheme they prefer. | Control | CP.6 |
| CN.9 | The operator needs the firmware to apply input-shaping (center deadband and a response curve) between raw controller input and per-side wheel command, so small or aggressive stick motions translate predictably to motion. | Control | CP.6 |
| CN.10 | The operator needs the firmware to display the current operating mode (PAIRING / DISARMED / ARMED / FAULT) on the status LED using distinct, distinguishable patterns. | Information | CP.7 |
| CN.11 | The operator needs the firmware to emit a complete snapshot of robot state (mode, controller link, battery voltage, active fault flags, last input, last output) over USB-serial on request. | Information | CP.7, CP.9 |
| CN.12 | The operator needs the firmware to expose runtime get/set commands for tunable parameters (deadband, max power, slew, response curve, mapping mode) over the USB-serial console. | Construction | CP.8 |
| CN.13 | The operator needs the firmware to persist tuned parameter values to non-volatile storage so they survive reboots and battery swaps. | Construction | CP.8 |
| CN.14 | The operator needs the firmware to emit a periodic telemetry stream over USB-serial (mode, link, battery voltage, active faults, current input, current output) at a configurable rate, for post-match review. | Information | CP.9 |
| CN.15 | The operator needs the firmware to require a deliberate, multi-button controller action (unlikely to occur accidentally) to enter the ARMED state, so the robot cannot transition from DISARMED to ARMED through incidental input. | Control | CP.3, CP.5 |

## Outcome Class Distribution

| Class | Count | IDs |
|-------|------:|-----|
| Control | 10 | CN.1, CN.2, CN.3, CN.4, CN.5, CN.6, CN.7, CN.8, CN.9, CN.15 |
| Information | 3 | CN.10, CN.11, CN.14 |
| Construction | 2 | CN.12, CN.13 |
| Entertainment | 0 | — |
| **Total** | **15** | |

Heavy weighting toward **Control** is expected: this firmware's primary purpose is to drive motors in response to operator input. Information needs cluster around state feedback and telemetry; Construction needs are limited to runtime parameter tuning.

## Zigzag Validation (CP → CN Coverage) — MANDATORY GATE

|       | CN.1 | CN.2 | CN.3 | CN.4 | CN.5 | CN.6 | CN.7 | CN.8 | CN.9 | CN.10 | CN.11 | CN.12 | CN.13 | CN.14 | CN.15 |
|-------|:----:|:----:|:----:|:----:|:----:|:----:|:----:|:----:|:----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
| CP.1  |  C   |      |      |      |      |      |      |      |      |       |       |       |       |       |       |
| CP.2  |      |  C   |      |      |      |      |      |      |      |       |       |       |       |       |       |
| CP.3  |      |      |  C   |  C   |      |      |      |      |      |       |       |       |       |       |   P   |
| CP.4  |      |      |      |      |  C   |      |      |      |      |       |       |       |       |       |       |
| CP.5  |      |      |      |      |      |  C   |  C   |      |      |       |       |       |       |       |   P   |
| CP.6  |      |      |      |      |      |      |      |  C   |  C   |       |       |       |       |       |       |
| CP.7  |      |      |      |      |      |      |      |      |      |   C   |   P   |       |       |       |       |
| CP.8  |      |      |      |      |      |      |      |      |      |       |       |   C   |   C   |       |       |
| CP.9  |      |      |      |      |      |      |      |      |      |       |   P   |       |       |   C   |       |

**Legend:** **C** = Complete coverage  •  **P** = Partial coverage (contributes but not the primary source)

### Coverage Check

| CP | Has ≥1 CN? | Notes |
|----|:---------:|-------|
| CP.1 Low input latency | ✅ | CN.1 sets the round-trip budget |
| CP.2 Full torque on demand | ✅ | CN.2 forbids firmware-imposed caps below the configured max |
| CP.3 Safe stop on command / link loss | ✅ | CN.3 covers operator command; CN.4 covers link loss — distinct mechanisms |
| CP.4 Reliable BLE link | ✅ | CN.5 covers auto-(re)pair and reconnect |
| CP.5 Protect hardware from faults | ✅ | CN.6 covers brownout; CN.7 covers the disarmed-off invariant. *Thermal / over-current protection deferred per open question in 00-context.md* |
| CP.6 Intuitive tank controls | ✅ | CN.8 chooses the mapping; CN.9 shapes the response |
| CP.7 Visible state feedback | ✅ | CN.10 (LED) primary; CN.11 (serial snapshot) supplementary |
| CP.8 Tuning without reflash | ✅ | CN.12 exposes runtime tuning; CN.13 makes it persistent |
| CP.9 Diagnostic telemetry | ✅ | CN.14 periodic stream primary; CN.11 on-request snapshot supplementary |

**Result:** every CP has at least one **C**-level CN. No orphan problems. ✅

### Orphan-CN Check

Does every CN trace back to at least one CP?

| CN | Traces back to | OK? |
|----|----------------|:---:|
| CN.1–CN.14 | See "Traces To" column above | ✅ all CNs cite ≥1 CP |

No orphan CNs. ✅

## Quality Gate (Step 3)

- [x] Every CP has at least one CN (zigzag CP → CN complete)
- [x] All CNs use `[Subject] needs [system] to [Verb] [Object] [Condition]` notation
- [x] Outcome classes assigned to every CN
- [x] No orphan CNs (every CN traces to ≥1 CP)
- [x] No solutions baked into CN statements (no GPIO pins, no PWM frequencies, no library names)
- [x] File saved: `03-customer-needs.md`
- [x] **Zigzag validation performed** (matrix above)

## Notes

- **CN.4 vs CN.7:** these look similar but they answer different questions. CN.4 is reactive ("when the link dies, stop"); CN.7 is invariant ("while DISARMED, motors are off regardless of input"). Step 5 will likely produce separate FRs for each.
- **CN.6 currently uses pack-voltage only.** Adding driver thermal/current protection would mean adding a new CN (e.g., CN.15) and tracing it to CP.5. That decision is parked as an open question.
- **CN.11 vs CN.14:** snapshot-on-request vs. periodic stream. Two separate CNs because they have different triggers (operator vs. timer) and different bandwidth profiles. Useful to keep separate for testability.

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20 (added CN.15: deliberate-arming requirement, traces to CP.3 + CP.5)*
