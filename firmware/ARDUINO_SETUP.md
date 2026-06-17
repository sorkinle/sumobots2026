# Arduino IDE setup & flashing

How to build and flash the sumobot firmware (e.g. `sumobot_connection_test/`) onto an ESP32 with the
**Arduino IDE**. The one thing that trips people up: **Bluepad32 is installed as a *board package*, not a
library** — it needs a special build of the ESP32 core with the Bluetooth stack, and you must select the
board from the **"ESP32 + Bluepad32"** group or `#include <Bluepad32.h>` won't resolve.

## Prerequisites

- **Arduino IDE 2.x** (<https://www.arduino.cc/en/software>).
- An **ESP32 dev board** (ESP-WROOM-32 / ESP32 DevKitC) and a USB data cable.
- A **Bluetooth-LE Xbox controller** — Xbox Series X|S, or an Xbox One controller with Bluetooth and updated
  firmware. The older 2.4 GHz "dongle" controllers do **not** pair.

## 1. Add the two board-manager URLs

**File → Preferences → Additional Boards Manager URLs**, and add both (comma-separated, or one per line):

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
```

## 2. Install both board packages

**Tools → Board → Boards Manager**, then install:

- **`esp32`** (Espressif Systems), and
- **`ESP32 + Bluepad32`** (Ricardo Quesada).

## 3. Install the USB-serial driver (Windows)

If no COM port shows up later, install the driver for your board's USB-UART chip:

- **CP210x** (Silicon Labs) — most DevKitC boards, or
- **CH340/CH341** (WCH) — many clones.

## 4. Open the sketch

**File → Open →** `firmware/sumobot_connection_test/sumobot_connection_test.ino`. The helper files
(`drive_logic.*`, `motor_pair.*`) open with it automatically — they live in the same folder.

## 5. Select the board (the important step)

**Tools → Board → ESP32 + Bluepad32 Arduino → ESP32 Dev Module** (or the exact board you have).

> Pick it from the **"ESP32 + Bluepad32 Arduino"** group — **not** the plain "ESP32 Arduino" group, or the
> build fails with `Bluepad32.h: No such file or directory`.

Leave the defaults (Upload Speed 921600 is fine). Then **Tools → Port →** select your ESP32's COM port.

## 6. Upload

Click **Upload** (→). On first flash some boards need you to hold the **BOOT** button while it says
"Connecting…". When it finishes, open **Tools → Serial Monitor** at **115200 baud**.

## 7. Pair the controller

Turn the controller on, then press its **pair button** until the Xbox logo flashes quickly. The Serial
Monitor should print `Controller connected`.

## 8. First run — smoke-test order (safe)

1. **With no motor drivers connected**, hold **LB + RB** and move the sticks. The Serial Monitor should show
   `armed=1` and the per-side duties changing. This proves Controller → ESP32 with zero risk.
2. **Then wire the BTS7960 driver(s)** (pin map in [`README.md`](README.md)), robot **up on blocks** (wheels
   off the ground), and repeat. Motors run **only while LB + RB are held**; release to stop.

A single motor is enough to test — connect it to one side's driver; the other side's pins just toggle
harmlessly. If a side spins the wrong way, swap that side's `M+/M−` (or its RPWM/LPWM wires).

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Bluepad32.h: No such file or directory` | You selected a plain "ESP32 Arduino" board. Re-select it under **ESP32 + Bluepad32 Arduino** (step 5). |
| No COM port in **Tools → Port** | Install the CP210x/CH340 driver (step 3); try a different USB cable (must be data, not charge-only). |
| Upload stuck at "Connecting…" | Hold the **BOOT** button during upload; lower Upload Speed to 115200. |
| Controller never connects | Use a BLE-capable Xbox controller; update its firmware via the Xbox Accessories app. To clear stale pairings, uncomment `BP32.forgetBluetoothKeys();` in `setup()`, upload once, then re-comment. |
| Motor never spins | You must hold **LB + RB** to arm (the `EN` pin only goes HIGH when armed); check common ground between ESP32 and driver, and that the driver's logic VCC is powered. |

For host-side unit tests (no board needed), see the "Host tests" section of [`README.md`](README.md).
