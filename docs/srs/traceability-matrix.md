# Traceability Matrix — ESP32 Sumobot Firmware

> **Method:** Problem-Based SRS (Gorski & Stadzisz)
> **Trace chain:** CP → CN → FR / NFR (every requirement traces back to a real business problem)
> **Status:** Step 5 final — zigzag validation across the full chain

## 1. CP → CN Coverage

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

**Legend:** **C** = Complete coverage  •  **P** = Partial / contributory

## 2. CN → FR Coverage

|        | FR-001 | FR-002 | FR-003 | FR-004 | FR-005 | FR-006 | FR-007 | FR-008 | FR-009 | FR-010 | FR-011 | FR-012 | FR-013 | FR-014 | FR-015 | FR-016 | FR-017 | FR-018 | FR-019 |
|--------|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|:------:|
| CN.1   |   C    |   C    |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |
| CN.2   |        |        |   C    |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |
| CN.3   |        |        |        |   C    |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |
| CN.4   |        |        |        |        |        |   C    |        |        |        |        |        |        |        |        |        |        |        |        |        |
| CN.5   |        |        |        |        |        |        |   C    |   C    |        |        |        |        |        |        |        |        |        |        |        |
| CN.6   |        |        |        |        |        |        |        |        |   C    |        |        |        |        |        |        |        |        |        |   P    |
| CN.7   |        |        |        |        |        |        |        |        |        |   C    |        |        |        |        |        |        |        |        |   P    |
| CN.8   |        |        |        |        |        |        |        |        |        |        |   C    |        |        |        |        |        |        |        |        |
| CN.9   |        |        |        |        |        |        |        |        |        |        |        |   C    |   C    |        |        |        |        |        |        |
| CN.10  |        |        |        |        |        |        |        |        |        |        |        |        |        |   C    |        |        |        |        |        |
| CN.11  |        |        |        |        |        |        |        |        |        |        |        |        |        |        |   C    |        |        |        |        |
| CN.12  |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |   C    |        |        |        |
| CN.13  |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |   C    |        |        |
| CN.14  |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |        |   C    |        |
| CN.15  |        |        |        |        |   C    |        |        |        |        |        |        |        |        |        |        |        |        |        |        |

## 3. End-to-End Trace (FR → CN → CP)

| FR | CN | CP | Title |
|----|----|----|-------|
| FR-001 | CN.1 | CP.1 | Input loop rate ≥ 100 Hz |
| FR-002 | CN.1 | CP.1 | HID → PWM latency ≤ 50 ms |
| FR-003 | CN.2 | CP.2 | Max-throttle pass-through |
| FR-004 | CN.3 | CP.3 | Emergency-stop button (B) |
| FR-005 | CN.15 | CP.3, CP.5 | Arming combo (LB+RB+A hold ≥ 1 s) |
| FR-006 | CN.4 | CP.3 | Link-loss failsafe (250 ms) |
| FR-007 | CN.5 | CP.4 | Initial pairing |
| FR-008 | CN.5 | CP.4 | Auto-reconnect |
| FR-009 | CN.6 | CP.5 | Brownout detection |
| FR-010 | CN.7 | CP.5 | DISARMED motor-off invariant |
| FR-011 | CN.8 | CP.6 | Mapping mode selector (arcade/tank) |
| FR-012 | CN.9 | CP.6 | Center deadband |
| FR-013 | CN.9 | CP.6 | Response curve |
| FR-014 | CN.10 | CP.7 | LED patterns |
| FR-015 | CN.11 | CP.7, CP.9 | On-demand status snapshot |
| FR-016 | CN.12 | CP.8 | Tunable-parameter console commands |
| FR-017 | CN.13 | CP.8 | NVS persistence |
| FR-018 | CN.14 | CP.9 | Periodic telemetry stream |
| FR-019 | CN.6, CN.7 | CP.5 | Fault-clear action |

## 4. NFR → CN / CP Trace

| NFR | CN(s) | CP(s) | Applies To FRs |
|-----|-------|-------|----------------|
| NFR-001 | CN.1 | CP.1 | FR-001, FR-002, FR-003 |
| NFR-002 | CN.5 | CP.4 | FR-008 |
| NFR-003 | CN.7 | CP.3, CP.5 | FR-010 |
| NFR-004 | CN.12, CN.13 | CP.8 | FR-016, FR-017 |
| NFR-005 | CN.10 | CP.7 | FR-014 |
| NFR-006 | — | (indirect) | (all) |

## 5. Zigzag Validation (Final)

### CP coverage — does every CP have ≥ 1 CN?

| CP | ≥1 CN? | CNs |
|----|:------:|-----|
| CP.1 | ✅ | CN.1 |
| CP.2 | ✅ | CN.2 |
| CP.3 | ✅ | CN.3, CN.4, CN.15 |
| CP.4 | ✅ | CN.5 |
| CP.5 | ✅ | CN.6, CN.7, CN.15 |
| CP.6 | ✅ | CN.8, CN.9 |
| CP.7 | ✅ | CN.10, CN.11 |
| CP.8 | ✅ | CN.12, CN.13 |
| CP.9 | ✅ | CN.11, CN.14 |

**Result:** No orphan CPs. ✅

### CN coverage — does every CN have ≥ 1 FR?

| CN | ≥1 FR? | FRs |
|----|:------:|-----|
| CN.1 | ✅ | FR-001, FR-002 |
| CN.2 | ✅ | FR-003 |
| CN.3 | ✅ | FR-004 |
| CN.4 | ✅ | FR-006 |
| CN.5 | ✅ | FR-007, FR-008 |
| CN.6 | ✅ | FR-009, FR-019 |
| CN.7 | ✅ | FR-010, FR-019 |
| CN.8 | ✅ | FR-011 |
| CN.9 | ✅ | FR-012, FR-013 |
| CN.10 | ✅ | FR-014 |
| CN.11 | ✅ | FR-015 |
| CN.12 | ✅ | FR-016 |
| CN.13 | ✅ | FR-017 |
| CN.14 | ✅ | FR-018 |
| CN.15 | ✅ | FR-005 |

**Result:** No orphan CNs. ✅

### Orphan FR check — does every FR trace to ≥ 1 CN?

Every FR's "Traces To" table includes at least one CN (see individual FR files). ✅

### Orphan NFR check

Every NFR traces to at least one CN except NFR-006 (resource budget), which is intentionally architectural and supports the overall maintainability/recovery story implicit in CP.5. Acceptable.

## 6. Summary Counts

| Artifact | Count |
|----------|------:|
| Customer Problems | 9 (5 Obligation, 2 Expectation, 2 Hope) |
| Customer Needs | 15 (10 Control, 3 Information, 2 Construction) |
| Functional Requirements | 19 (11 Must, 8 Should) |
| Non-Functional Requirements | 6 (4 Must, 2 Should) |

**Specification type:** Slightly redundant — multiple FRs per CN in some cases (CN.5, CN.6, CN.7, CN.9), which is expected and aids testability. No orphans in either direction.

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
