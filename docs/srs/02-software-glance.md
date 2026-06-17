# Step 2: Software Glance (SG)

> **Purpose:** Initial abstract solution view for the ESP32 sumobot firmware.
> Defines the firmware's **boundary**, its **external actors**, and the **internal components** that, working together, will address the Customer Problems from Step 1.

## System Boundary

The system under specification is **the firmware running on the ESP32**. Everything else — battery, buck converter, motor drivers, motors, chassis, opponent, ring surface — is **outside the boundary** and treated as an external actor or environmental condition.

```
┌─────────────────────────────────────────────┐
│   OUTSIDE BOUNDARY (environment / hardware) │
│                                             │
│   Operator   Xbox Controller   Opponent     │
│   Battery    BTS7960 drivers   Motors       │
│   Chassis    USB host (PC)     LED          │
└─────────────────────────────────────────────┘
                      ▲
                      │ signals, BLE, USB-serial
                      ▼
        ┌────────────────────────────┐
        │   ESP32 SUMOBOT FIRMWARE   │  ← in scope
        │   (this specification)     │
        └────────────────────────────┘
```

## External Actors & Interfaces

| Actor | Direction | Interface | Carried Data |
|-------|-----------|-----------|--------------|
| **Operator** | Indirect (via controller and LED) | n/a | Intent (drive, arm, e-stop); reads LED state |
| **Xbox Series controller** | ↔ Firmware | Bluetooth LE (HID GATT) | Stick / trigger / button events; link state |
| **BTS7960 pair (Left + Right)** | ← Firmware | ESP32 GPIO: RPWM, LPWM, R_EN, L_EN per side | Direction + PWM duty + enable |
| **Battery (4S LiPo via divider)** | → Firmware | ESP32 ADC pin | Pack voltage (for brownout / low-batt) |
| **Status LED (onboard or GPIO LED)** | ← Firmware | GPIO | Visible state pattern |
| **USB host (PC) — bench only** | ↔ Firmware | USB CDC serial (UART0) | Telemetry stream out; tuning/admin commands in |
| **Motors (×4)** | (indirect) | via BTS7960 only | n/a — firmware never touches motors directly |

The firmware never directly controls the motors or the battery; it acts **through** the BTS7960 drivers and reads the battery **through** an ADC divider.

## Operating Modes (high-level state)

| Mode | Meaning |
|------|---------|
| **BOOT** | Power-on self-init; no motor activity possible |
| **PAIRING** | Waiting for the Xbox controller to connect / reconnect |
| **DISARMED** | Controller connected, but motor outputs are forced off |
| **ARMED** | Operator has explicitly armed the robot; drive commands take effect |
| **FAULT** | A safety condition (link loss, brownout, etc.) has forced motor-off; requires recovery |

Arming requires an explicit operator action; coming out of `BOOT`, `PAIRING`, or `FAULT` always lands in `DISARMED`, never directly in `ARMED`. This is a deliberate safety bias driven by CP.3 and CP.5.

## Internal Components (firmware)

| # | Component | Role |
|--:|-----------|------|
| 1 | **BLE Controller Interface** | Pairs / reconnects with the Xbox controller; parses HID gamepad reports into a normalized input record (sticks, triggers, buttons, link health). |
| 2 | **Drive Mixer** | Converts operator input into per-side wheel commands in the range −1.0 … +1.0 using a configurable mapping (arcade or tank). Applies deadband, throttle curve, and output clamp. |
| 3 | **Motor Output Driver** | Abstracts the BTS7960 pair. Given `(left_cmd, right_cmd, enabled)`, produces direction + PWM duty + enable signals on the assigned GPIOs. The only component that touches motor GPIOs. |
| 4 | **State Machine (Arm / Disarm)** | Owns mode transitions and gates the Motor Output Driver behind `ARMED`. Listens to operator arm/disarm/e-stop, link-loss events, and fault signals. |
| 5 | **Failsafe / Watchdog** | Monitors link liveness (heartbeat / timeout), pack voltage (brownout threshold), and optionally driver temperature / current. Asserts a fault signal that forces `FAULT` mode. |
| 6 | **Configuration Store** | Reads/writes tunable parameters (deadband, max power, slew, curve, pin map) from ESP32 NVS. Exposes get/set/save/load to the Serial Console. |
| 7 | **Status Indicator** | Drives the status LED with distinct patterns per mode (PAIRING, DISARMED, ARMED, FAULT). |
| 8 | **Telemetry / Serial Console** | Emits a periodic diagnostic stream (mode, link, Vbatt, faults, last input, output duties) over USB-serial; accepts simple text commands for tuning and inspection. |

## Internal Block Diagram

```
                ┌─────────────────────────────────────────────────────────────────┐
                │                       ESP32 Firmware                            │
                │                                                                  │
   Xbox ◄──BLE──┤ ① BLE Controller Interface                                       │
                │            │ normalized input                                    │
                │            ▼                                                     │
                │   ② Drive Mixer ◄──── ⑥ Configuration Store ◄──┐                │
                │            │ L/R cmd                              │              │
                │            ▼                                      │ params       │
                │   ④ State Machine ───gate (armed?)────►③ Motor Output Driver ───┼──► BTS7960 ×2
                │     ▲    ▲          ▲                                            │   (GPIO PWM/DIR/EN)
                │     │    │          │ fault                                      │
                │     │    └────── ⑤ Failsafe / Watchdog ◄──── Vbatt (ADC) ◄──────┼──── Battery divider
                │     │                  ▲                                         │
                │     │ link / heartbeat │                                         │
                │     │                                                            │
                │     └──────────────► ⑦ Status Indicator ───────────────────────┼──► LED
                │                                                                  │
                │              ⑧ Telemetry / Serial Console ◄── all components ──┼──► USB Serial ↔ PC
                └─────────────────────────────────────────────────────────────────┘
```

**Data-flow guarantees** (reflected later as FRs in Step 5):
- **Operator input** never reaches the motor outputs except through the State Machine's `ARMED` gate.
- **Failsafe** can force motor-off without cooperation from the input pipeline.
- **Configuration** affects only the Drive Mixer's shaping; it cannot override Arm/Disarm or Failsafe.
- **Telemetry** is read-only with respect to motor control; the Serial Console may set parameters but cannot directly command motors.

## How Each CP Will Be Addressed (preview only)

> Detailed coverage is the job of Step 3 (Customer Needs). This table just shows that the architecture has a plausible home for every problem.

| CP | Primary component(s) responsible |
|----|----------------------------------|
| CP.1 Low input latency | ① BLE Controller Interface, ② Drive Mixer, ③ Motor Output Driver (tight loop) |
| CP.2 Full torque on demand | ② Drive Mixer (clamp/curve), ③ Motor Output Driver (PWM range) |
| CP.3 Safe stop on command/link loss | ④ State Machine, ⑤ Failsafe |
| CP.4 Reliable BLE link | ① BLE Controller Interface (pair, reconnect, heartbeat) |
| CP.5 Protect hardware from faults | ⑤ Failsafe (brownout, optional thermal/current), ④ State Machine |
| CP.6 Intuitive tank controls | ② Drive Mixer (arcade / tank mapping, deadband, curve) |
| CP.7 Visible state feedback | ⑦ Status Indicator, ⑧ Serial Console |
| CP.8 Tuning without reflash | ⑥ Configuration Store, ⑧ Serial Console |
| CP.9 Diagnostic telemetry | ⑧ Telemetry / Serial Console, ⑤ Failsafe (fault flags) |

Every CP has at least one component on the hook — no orphan problems at the architecture level.

## Quality Gate (Step 2)

- [x] System boundary defined (firmware on ESP32; hardware is external)
- [x] Main actors and interfaces identified (table above)
- [x] High-level components described (8 components, single responsibility each)
- [x] Architecture has a plausible home for every CP (preview table)
- [x] File saved: `02-software-glance.md`

---
*Created: 2026-05-20*
*Last Updated: 2026-05-20*
