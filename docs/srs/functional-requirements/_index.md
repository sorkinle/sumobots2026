# Functional Requirements — Index

> **Spec:** ESP32 Sumobot Firmware
> **Method:** Problem-Based SRS (Gorski & Stadzisz)
> **Trace chain:** FR → CN → CP (every FR points back to a Customer Need and ultimately a Customer Problem)

## FR Summary

| FR ID | Title | Priority | Status | Traces To |
|-------|-------|----------|--------|-----------|
| FR-001 | Input Sample-and-Update Loop Rate | Must Have | Draft | CN.1 |
| FR-002 | HID-Report → PWM Latency | Must Have | Draft | CN.1 |
| FR-003 | Max-Throttle Pass-Through | Must Have | Draft | CN.2 |
| FR-004 | Emergency-Stop Button | Must Have | Draft | CN.3 |
| FR-005 | Arming Combination | Must Have | Draft | CN.15 |
| FR-006 | Link-Loss Failsafe | Must Have | Draft | CN.4 |
| FR-007 | Initial Pairing in PAIRING Mode | Must Have | Draft | CN.5 |
| FR-008 | Auto-Reconnect | Must Have | Draft | CN.5 |
| FR-009 | Brownout Detection | Must Have | Draft | CN.6 |
| FR-010 | DISARMED Motor-Off Invariant | Must Have | Draft | CN.7 |
| FR-011 | Input Mapping Mode Selector | Must Have | Draft | CN.8 |
| FR-012 | Center Deadband | Must Have | Draft | CN.9 |
| FR-013 | Response Curve | Should Have | Draft | CN.9 |
| FR-014 | Status LED Patterns | Must Have | Draft | CN.10 |
| FR-015 | On-Demand Status Snapshot | Should Have | Draft | CN.11 |
| FR-016 | Tunable-Parameter Console Commands | Should Have | Draft | CN.12 |
| FR-017 | NVS Persistence | Should Have | Draft | CN.13 |
| FR-018 | Periodic Telemetry Stream | Should Have | Draft | CN.14 |
| FR-019 | Fault-Clear Action | Should Have | Draft | CN.6, CN.7 |

**Counts:** 19 FRs — 11 Must, 8 Should, 0 Could, 0 Won't.

## FR → CN → CP Quick Trace

| FR | CN | CP |
|----|----|----|
| FR-001, FR-002 | CN.1 | CP.1 |
| FR-003 | CN.2 | CP.2 |
| FR-004 | CN.3 | CP.3 |
| FR-005 | CN.15 | CP.3, CP.5 |
| FR-006 | CN.4 | CP.3 |
| FR-007, FR-008 | CN.5 | CP.4 |
| FR-009, FR-019* | CN.6 | CP.5 |
| FR-010, FR-019* | CN.7 | CP.5 |
| FR-011 | CN.8 | CP.6 |
| FR-012, FR-013 | CN.9 | CP.6 |
| FR-014 | CN.10 | CP.7 |
| FR-015 | CN.11 | CP.7, CP.9 |
| FR-016 | CN.12 | CP.8 |
| FR-017 | CN.13 | CP.8 |
| FR-018 | CN.14 | CP.9 |

\* FR-019 (Fault-Clear Action) traces to both CN.6 (brownout entry) and CN.7 (DISARMED-on-exit invariant).

Full bidirectional matrix lives in `../traceability-matrix.md`.

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
