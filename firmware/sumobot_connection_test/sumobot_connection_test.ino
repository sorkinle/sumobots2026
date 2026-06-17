// sumobot_connection_test.ino
//
// Connection test ONLY: Xbox controller (Bluetooth LE) -> ESP32 -> BTS7960
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

// ---- Pin map (docs/design/implementation-notes.md section 2) --------------
constexpr int L_RPWM = 25, L_LPWM = 26, L_EN = 27;   // left side
constexpr int R_RPWM = 32, R_LPWM = 33, R_EN = 14;   // right side
constexpr uint8_t L_CH_R = 0, L_CH_L = 1;            // LEDC channels (core 2.x)
constexpr uint8_t R_CH_R = 2, R_CH_L = 3;

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

    g_left.begin(L_RPWM, L_LPWM, L_EN, L_CH_R, L_CH_L);
    g_right.begin(R_RPWM, R_LPWM, R_EN, R_CH_R, R_CH_L);

    // If pairing ever gets stuck during bring-up, uncomment once to wipe keys:
    // BP32.forgetBluetoothKeys();
    BP32.setup(&onConnect, &onDisconnect);
}

void loop() {
    BP32.update();

    ControllerSnapshot snap;  // default: invalid, all-zero
    bool armed = false;
    if (g_ctl && g_ctl->isConnected() && g_ctl->hasData() && g_ctl->isGamepad()) {
        snap = snapshotFrom(g_ctl);
        // Connection-test arming: hold BOTH shoulders to allow motion.
        armed = (snap.buttons & (btn::LB | btn::RB)) == (btn::LB | btn::RB);
    }

    const DriveOutput out = drive_pipeline(snap, g_params, armed);
    g_left.apply(out.left);
    g_right.apply(out.right);

    debugPrint(snap, armed, out);
    delay(10);  // ~100 Hz loop
}
