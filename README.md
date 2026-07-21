# sumobots2026

Specification-first repository for a 4WD ESP32 sumobot: Xbox controller over BLE (Bluepad32) drives two ZK-BM1 motor drivers (one per side, one channel per motor), powering four Pololu 37D motors.

## What is in this repo

| Area | Purpose |
|---|---|
| `docs/srs/` | Problem-Based SRS (the **WHAT**): context, traceability, FR/NFR set |
| `docs/design/implementation-notes.md` | Firmware design companion (the **HOW**): pin map, architecture, loop, subsystem sketches |
| `docs/design/chassis/` | Mechanical design notes, dimensions, and assembly references |
| `firmware/sumobot_connection_test/` | Working vertical slice: controller -> mixer -> motor PWM output |
| `firmware/test/` | Host C++ tests for Arduino-free logic |
| `models/` | Manufacturing/design assets: DXF cut sketches, STL parts, and Fusion archive |

## Current firmware status

The implemented firmware today is the **connection-test sketch** (`firmware/sumobot_connection_test/`), focused on proving the complete drive path:

1. BLE gamepad input via Bluepad32
2. Drive mixing + braking behavior in `drive_logic.*`
3. PWM output through `motor_pair.*` to both ZK-BM1 boards

It now accepts **any compatible controller** (no hardcoded MAC binding in the sketch).

## Firmware architecture (implemented slice)

| File | Role |
|---|---|
| `sumobot_connection_test.ino` | Device entry point: Bluepad32 callbacks, 100 Hz loop, input->pipeline->motor output |
| `drive_logic.h/.cpp` | Arduino-free control logic: stick mapping, slew, brake pulse behavior, output gating |
| `motor_pair.h/.cpp` | Hardware wrapper for ZK-BM1 signals (the PWM/GPIO writer) |
| `controller_binding.h/.cpp` | Arduino-free Bluetooth address helper/format functions + tests |

## Hardware and wiring baseline

- MCU: ESP-WROOM-32 dev board
- Input: Xbox BLE controller via Bluepad32
- Drive: 2x ZK-BM1 dual-channel H-bridge drivers
- Motors: 4x Pololu 37D (2 per side)
- Power: 3S LiPo -> drivers, plus LM2596 buck -> ESP32 VIN
- Pin map and setup details: `docs/design/implementation-notes.md` and `firmware/README.md`

## Build and run

Firmware uses **Arduino IDE** (not PlatformIO in this repo workflow).

1. Follow `firmware/ARDUINO_SETUP.md` to install board packages (`esp32` and `ESP32 + Bluepad32`) and USB-serial drivers.
2. Open `firmware/sumobot_connection_test/sumobot_connection_test.ino`.
3. Select a board under **ESP32 + Bluepad32 Arduino**.
4. Upload and open Serial Monitor at 115200.
5. Pair a BLE-capable controller and bench-test with wheels off the ground first.

## Host tests (TDD path)

The logic is intentionally split so core behavior can be tested off-board:

```sh
cd firmware/test
g++ -std=c++17 -I ../sumobot_connection_test test_drive_logic.cpp \
    ../sumobot_connection_test/drive_logic.cpp -o test_drive_logic
./test_drive_logic

g++ -std=c++17 -I ../sumobot_connection_test test_controller_binding.cpp \
    ../sumobot_connection_test/controller_binding.cpp -o test_controller_binding
./test_controller_binding
```

## Chassis and manufacturing documentation

### Materials used (current build)

- Chassis sheet material: **6 mm birch plywood**
- Front wedge: **3D printed**

### `models/` folder contents

| File | Type | Typical use |
|---|---|---|
| `Back_cut_sketch.dxf` | 2D CAD sketch | Back panel cutting profile |
| `Bottom_cut_sketch.dxf` | 2D CAD sketch | Base plate cutting profile |
| `Left_cut_sketch.dxf` | 2D CAD sketch | Left-side panel profile |
| `Right_cut_sketch.dxf` | 2D CAD sketch | Right-side panel profile |
| `Top_cut_sketch.dxf` | 2D CAD sketch | Top panel profile |
| `Wedge_support_cut_sketch.dxf` | 2D CAD sketch | Wedge support profile |
| `Left v2.stl` | 3D mesh | Printable left-side/aux part |
| `Right v2.stl` | 3D mesh | Printable right-side/aux part |
| `Sumobot v2.f3z` | Autodesk Fusion archive | Full assembly/source CAD package |

If you want the full mechanical rationale and assembly sequence, use:

- `docs/design/chassis/chassis-design.md`
- `docs/design/chassis/chassis-assembly.md`

## Spec-first traceability

The project keeps requirements and implementation intentionally separated:

- `docs/srs/` contains requirement intent and acceptance criteria (no code-level implementation details).
- `docs/design/implementation-notes.md` maps concrete design decisions back to FR/NFR identifiers.
