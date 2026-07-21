// sumobot_connection_test.ino
//
// Connection test ONLY: Xbox controller (Bluetooth LE) -> ESP32 -> ZK-BM1
// drivers -> motors. It proves the end-to-end drive path and nothing else
// (no full arm/disarm state machine or telemetry yet; the only persisted state
// is the drive state held in RAM while connected).
//
// CONTROLS: the robot drives whenever a controller is connected. Tap/hold A to
// stop — it fires a brief reverse "plug brake" pulse to kill momentum (the
// motors have no brake and otherwise coast), then holds stopped while still
// letting you turn in place. Motors stop immediately if the controller
// disconnects.
//
// PAIRING: open mode for this connection-test sketch. Any BLE gamepad that
// Bluepad32 recognizes may connect; only one active controller is used at a
// time for driving.
//
// SAFETY: there is no hold-to-arm gesture — motion is live while connected.
// Always bench-test with the robot up on blocks (wheels off the ground) first
// and at low max_power. See firmware/README.md and docs/design/implementation-notes.md.

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
    const ControllerProperties props = ctl->getProperties();
    if (g_ctl == nullptr) {
        g_ctl = ctl;
        Serial.printf("Controller connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      props.btaddr[0], props.btaddr[1], props.btaddr[2],
                      props.btaddr[3], props.btaddr[4], props.btaddr[5]);
        return;
    }

    if (g_ctl != ctl) {
        Serial.printf("Ignoring extra controller: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      props.btaddr[0], props.btaddr[1], props.btaddr[2],
                      props.btaddr[3], props.btaddr[4], props.btaddr[5]);
        ctl->disconnect();
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
    Serial.println("Sumobot connection test - drives while connected; hold A to stop");

    // Drive power: default to full scale. Force it to max if it came up unset
    // (e.g. a future NVS/config load returning 0) so the robot never runs at
    // zero power by accident.
    if (g_params.max_power == 0) {
        g_params.max_power = 1023;  // 10-bit PWM full scale
    }

    g_left.begin(L_M1_FWD, L_M1_REV, L_M2_FWD, L_M2_REV, 0, 1, 2, 3);
    g_right.begin(R_M1_FWD, R_M1_REV, R_M2_FWD, R_M2_REV, 4, 5, 6, 7);

    BP32.setup(&onConnect, &onDisconnect);
    BP32.enableNewBluetoothConnections(true);
    Serial.println("Waiting for any compatible controller to pair/connect");
}

void loop() {
    BP32.update();

    // Static: hold last-known controller state across frames where hasData() is
    // false (BT sends at ~20-50 Hz but loop runs at ~100 Hz). Without this the
    // motors get pulsed ON/OFF causing flickering.
    static ControllerSnapshot snap;  // default: invalid, all-zero
    static bool armed = false;
    static DriveState drive_state;   // per-side slew-limiter + brake state
    static uint32_t last_loop_ms = 0;

    // Elapsed time since the previous loop, for the button-A reverse-brake timer.
    const uint32_t now_ms = millis();
    const uint32_t dt_ms  = last_loop_ms ? (now_ms - last_loop_ms) : 0;
    last_loop_ms = now_ms;

    if (g_ctl && g_ctl->isConnected()) {
        if (g_ctl->hasData() && g_ctl->isGamepad()) {
            snap = snapshotFrom(g_ctl);
            armed = true;
        }
        // else: no fresh frame this loop (normal at ~100 Hz vs ~20-50 Hz BT) —
        // keep the last snapshot so the motors don't stutter.
    } else {
        // Gamepad disconnected — stop. The single ARMED gate zeroes output.
        snap = ControllerSnapshot{};
        armed = false;
    }

    const DriveOutput out = drive_pipeline(snap, g_params, armed, dt_ms, drive_state);
    g_left.apply(out.left);
    g_right.apply(out.right);

    debugPrint(snap, armed, out);
    delay(10);  // ~100 Hz loop
}
