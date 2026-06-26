// sumobot_connection_test.ino
//
// Connection test ONLY: Xbox controller (Bluetooth LE) -> ESP32 -> ZK-BM1
// drivers -> motors. It proves the end-to-end drive path and nothing else
// (no arm/disarm state machine, failsafe, telemetry, or NVS yet).
//
// SAFETY: motors run ONLY while you hold LB + RB together (the arm gesture for
// this test). Release either shoulder and both sides stop immediately. Always
// bench-test with the robot up on blocks (wheels off the ground) first and at
// low max_power. See firmware/README.md and docs/design/implementation-notes.md.

#include <Bluepad32.h>

#include "drive_logic.h"
#include "motor_pair.h"

// ---- Pin map (ZK-BM1 dual-channel drivers) --------------------------------
// Left ZK-BM1:  ESP32 GPIO -> Driver pin
//   GPIO 4  -> IN1    GPIO 18 -> IN2    GPIO 19 -> IN3    GPIO 21 -> IN4
constexpr int L_M1_FWD = 4,  L_M1_REV = 18;
constexpr int L_M2_FWD = 19, L_M2_REV = 21;
// Right ZK-BM1: ESP32 GPIO -> Driver pin
//   GPIO 25 -> IN1    GPIO 26 -> IN2    GPIO 32 -> IN3    GPIO 33 -> IN4
constexpr int R_M1_FWD = 25, R_M1_REV = 26;
constexpr int R_M2_FWD = 32, R_M2_REV = 33;

MotorPair g_left, g_right;
DriveParams g_params;            // compiled-in defaults (see drive_logic.h)
ControllerPtr g_ctl = nullptr;

static void onConnect(ControllerPtr ctl) {
    if (g_ctl == nullptr) {
        g_ctl = ctl;
        Serial.println("Controller connected");
    }
}

static void onDisconnect(ControllerPtr ctl) {
    if (g_ctl == ctl) {
        g_ctl = nullptr;
        Serial.println("Controller disconnected");
    }
}

static ControllerSnapshot snapshotFrom(ControllerPtr ctl) {
    ControllerSnapshot s;
    // Bluepad32 reports stick-up as negative; flip Y so +ly = forward.
    s.lx = ctl->axisX();
    s.ly = -ctl->axisY();
    s.rx = ctl->axisRX();
    s.ry = -ctl->axisRY();
    s.lt = ctl->brake();      // L2, 0..1023
    s.rt = ctl->throttle();   // R2, 0..1023
    if (ctl->a())  s.buttons |= btn::A;
    if (ctl->b())  s.buttons |= btn::B;
    if (ctl->x())  s.buttons |= btn::X;
    if (ctl->y())  s.buttons |= btn::Y;
    if (ctl->l1()) s.buttons |= btn::LB;
    if (ctl->r1()) s.buttons |= btn::RB;
    s.last_event_ms = millis();
    s.valid = true;
    return s;
}

static void debugPrint(const ControllerSnapshot& s, bool armed, const DriveOutput& o) {
    static uint32_t last = 0;
    const uint32_t now = millis();
    if (now - last < 250) return;
    last = now;
    Serial.printf("armed=%d ly=%d rx=%d  L(r=%u l=%u en=%d)  R(r=%u l=%u en=%d)\n",
                  armed ? 1 : 0, s.ly, s.rx,
                  o.left.rpwm_duty, o.left.lpwm_duty, o.left.enable,
                  o.right.rpwm_duty, o.right.lpwm_duty, o.right.enable);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Sumobot connection test - hold LB+RB to arm");

    g_left.begin(L_M1_FWD, L_M1_REV, L_M2_FWD, L_M2_REV, 0, 1, 2, 3);
    g_right.begin(R_M1_FWD, R_M1_REV, R_M2_FWD, R_M2_REV, 4, 5, 6, 7);

    // If pairing ever gets stuck during bring-up, uncomment once to wipe keys:
    // BP32.forgetBluetoothKeys();
    BP32.setup(&onConnect, &onDisconnect);
}

void loop() {
    BP32.update();

    // Static: hold last-known controller state across frames where hasData() is
    // false (BT sends at ~20-50 Hz but loop runs at ~100 Hz). Without this the
    // motors get pulsed ON/OFF causing flickering.
    static ControllerSnapshot snap;  // default: invalid, all-zero
    static bool armed = false;
    static uint32_t last_data_ms = 0;

    constexpr uint32_t kTimeoutMs = 5000;  // failsafe: stop if no data for 5 seconds

    if (g_ctl && g_ctl->isConnected()) {
        if (g_ctl->hasData() && g_ctl->isGamepad()) {
            snap = snapshotFrom(g_ctl);
            armed = (snap.buttons & (btn::LB | btn::RB)) == (btn::LB | btn::RB);
            last_data_ms = millis();
        } else if (last_data_ms > 0 && millis() - last_data_ms > kTimeoutMs) {
            // No fresh data for too long — failsafe stop.
            snap = ControllerSnapshot{};
            armed = false;
        }
    } else {
        // Controller disconnected — zero everything.
        snap = ControllerSnapshot{};
        armed = false;
        last_data_ms = 0;
    }

    const DriveOutput out = drive_pipeline(snap, g_params, armed);
    g_left.apply(out.left);
    g_right.apply(out.right);

    debugPrint(snap, armed, out);
    delay(10);  // ~100 Hz loop
}
