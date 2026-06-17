# Step 1: Customer Problems (CP)

> **Domain:** Hobby combat robotics — ESP32-driven, Xbox-controlled, 4-motor tank-drive push-only sumobot
> **Scope:** Firmware only (mechanical/electrical are constraints, not problems)
> **Method:** Problem-Based SRS (Gorski & Stadzisz)

## Identified Problems

| ID | Statement | Class |
|----|-----------|-------|
| CP.1 | The operator must be able to drive the robot in real time with imperceptible input lag, otherwise the operator cannot react to opponent movement and loses match control. | Obligation |
| CP.2 | The robot must deliver full available motor torque to both wheels on a side when the operator commands maximum throttle, otherwise it cannot win pushing contests against comparably-sized opponents. | Obligation |
| CP.3 | The robot must stop all motors immediately on operator command or upon loss of the controller link, otherwise it becomes uncontrolled, runs out of the ring, or damages itself, the opponent, or bystanders. | Obligation |
| CP.4 | The robot must maintain a reliable wireless link to the Xbox controller for the duration of a match, otherwise the match is effectively forfeited due to loss of control. | Obligation |
| CP.5 | The robot must protect its motors, drivers, and battery from foreseeable electrical and thermal failure modes (stall, brownout, over-temperature, disarmed state), otherwise the hardware investment is destroyed in a single match. | Obligation |
| CP.6 | The operator expects tank-style stick controls that map predictably and symmetrically into forward, reverse, and pivot motion, otherwise piloting is unintuitive and the operator makes input errors under pressure. | Expectation |
| CP.7 | The operator expects clear local feedback on the robot's current state (powered, paired, armed, faulted), otherwise they cannot tell whether the robot is ready to drive or hung in an unsafe state. | Expectation |
| CP.8 | The operator hopes to tune driving parameters (stick deadband, maximum power, acceleration slew, throttle curve) without recompiling and re-flashing the firmware, otherwise iteration on the robot's "feel" is slow and error-prone. | Hope |
| CP.9 | The operator hopes the firmware emits diagnostic telemetry (battery voltage, fault flags, link quality, throttle history) for post-match review, otherwise debugging match failures is guesswork. | Hope |

## Classification Summary

| Class | Count | IDs |
|-------|------:|-----|
| Obligation | 5 | CP.1, CP.2, CP.3, CP.4, CP.5 |
| Expectation | 2 | CP.6, CP.7 |
| Hope | 2 | CP.8, CP.9 |
| **Total** | **9** | |

## Notes

- **Push-only** is treated as a scope constraint (in `00-context.md`), not a problem — there is no weapon-control problem to solve.
- **CP.3 (safe stop) and CP.4 (link reliability)** together form the safety contract. They are deliberately separate so a future zigzag can verify each is covered by independent CNs/FRs.
- **CP.5** is intentionally broad; it will likely decompose into per-fault-mode CNs in Step 3 (e.g., brownout handling, motor-off-when-disarmed, optional current/thermal limits).
- **No solution language** has been embedded in the problem statements (no mention of PWM frequency, BLE library, specific GPIOs, etc.) — those belong in Steps 2–5.

## Quality Gate (Step 1)

- [x] All CPs use the structured `[Subject] [must/expects/hopes] [Object] [Penalty]` notation
- [x] Classifications assigned (Obligation / Expectation / Hope)
- [x] No solutions embedded in problem statements
- [x] File saved: `01-customer-problems.md`

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
