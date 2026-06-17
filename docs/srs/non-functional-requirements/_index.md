# Non-Functional Requirements — Index

> **Spec:** ESP32 Sumobot Firmware
> NFRs express **quality attributes** (how well the system must behave) rather than discrete behaviors. Each NFR ties to one or more FRs that implement the behavior, and traces back to a CN/CP.

## NFR Summary

| NFR ID | Title | Category | Priority | Status | Applies To FRs |
|--------|-------|----------|----------|--------|----------------|
| NFR-001 | Input-to-Output Latency Budget | Performance | Must Have | Draft | FR-001, FR-002, FR-003 |
| NFR-002 | BLE Reconnect Reliability | Reliability | Must Have | Draft | FR-008 |
| NFR-003 | ARMED-Gate Safety Invariant | Safety | Must Have | Draft | FR-010 |
| NFR-004 | Runtime Tunability Without Reflash | Maintainability | Must Have | Draft | FR-016, FR-017 |
| NFR-005 | LED Pattern Distinguishability | Usability | Should Have | Draft | FR-014 |
| NFR-006 | Firmware Flash Footprint | Resource | Should Have | Draft | (all) |

**Counts:** 6 NFRs — 4 Must, 2 Should.

## NFR → CN → CP Quick Trace

| NFR | CN(s) | CP(s) |
|-----|-------|-------|
| NFR-001 | CN.1 | CP.1 |
| NFR-002 | CN.5 | CP.4 |
| NFR-003 | CN.7 | CP.3, CP.5 |
| NFR-004 | CN.12, CN.13 | CP.8 |
| NFR-005 | CN.10 | CP.7 |
| NFR-006 | — | (indirect, supports future CP.5 OTA recovery) |

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
