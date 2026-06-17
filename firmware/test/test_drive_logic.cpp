// Host-side unit tests for the hardware-independent drive logic.
// Self-contained: no test framework, compiles with any C++17 compiler.
//
//   g++ -std=c++17 -I ../sumobot_connection_test test_drive_logic.cpp \
//       ../sumobot_connection_test/drive_logic.cpp -o test_drive_logic && ./test_drive_logic
//
// See firmware/README.md for details.

#include <cstdio>

#include "drive_logic.h"

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_fails;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

#define CHECK_EQ(expected, actual)                                            \
    do {                                                                      \
        ++g_checks;                                                           \
        long long _e = (long long)(expected);                                 \
        long long _a = (long long)(actual);                                   \
        if (_e != _a) {                                                       \
            ++g_fails;                                                        \
            std::printf("  FAIL %s:%d  %s == %s (expected %lld, got %lld)\n", \
                        __FILE__, __LINE__, #expected, #actual, _e, _a);      \
        }                                                                     \
    } while (0)

static ControllerSnapshot snap(int16_t ly, int16_t rx, int16_t ry) {
    ControllerSnapshot s;
    s.ly = ly;
    s.rx = rx;
    s.ry = ry;
    s.valid = true;
    return s;
}

// ---- mixer ----------------------------------------------------------------
static void test_mixer() {
    DriveParams arcade;  // arcade, deadband 40, max_power 700

    CHECK_EQ(0, mixer_compute(snap(0, 0, 0), arcade).left);
    CHECK_EQ(0, mixer_compute(snap(0, 0, 0), arcade).right);

    // within deadband => suppressed
    CHECK_EQ(0, mixer_compute(snap(30, 20, 0), arcade).left);
    CHECK_EQ(0, mixer_compute(snap(30, 20, 0), arcade).right);

    // full forward drives both sides equally
    CHECK_EQ(700, mixer_compute(snap(512, 0, 0), arcade).left);
    CHECK_EQ(700, mixer_compute(snap(512, 0, 0), arcade).right);

    // pivot right: sides oppose
    CHECK_EQ(700, mixer_compute(snap(0, 512, 0), arcade).left);
    CHECK_EQ(-700, mixer_compute(snap(0, 512, 0), arcade).right);

    // forward + full steer must clamp to max_power, not overflow
    CHECK_EQ(700, mixer_compute(snap(512, 512, 0), arcade).left);
    CHECK_EQ(0, mixer_compute(snap(512, 512, 0), arcade).right);

    DriveParams tank;
    tank.mapping_mode = MAPPING_TANK;
    CHECK_EQ(700, mixer_compute(snap(512, 0, -512), tank).left);
    CHECK_EQ(-700, mixer_compute(snap(512, 0, -512), tank).right);
}

// ---- motor output (the ARMED gate) ----------------------------------------
static void test_motor_output() {
    MotorSignals fwd = motor_output_for(300, true);
    CHECK(fwd.enable);
    CHECK_EQ(300, fwd.rpwm_duty);
    CHECK_EQ(0, fwd.lpwm_duty);

    MotorSignals rev = motor_output_for(-300, true);
    CHECK(rev.enable);
    CHECK_EQ(0, rev.rpwm_duty);
    CHECK_EQ(300, rev.lpwm_duty);

    MotorSignals zero = motor_output_for(0, true);
    CHECK(zero.enable);
    CHECK_EQ(0, zero.rpwm_duty);
    CHECK_EQ(0, zero.lpwm_duty);

    // disarmed forces everything off even at full throttle
    MotorSignals off = motor_output_for(1023, false);
    CHECK(!off.enable);
    CHECK_EQ(0, off.rpwm_duty);
    CHECK_EQ(0, off.lpwm_duty);
}

// ---- whole pipeline: Controller -> ESP32 -> motor-driver signals ----------
static void test_pipeline() {
    DriveParams p;

    DriveOutput fwd = drive_pipeline(snap(512, 0, 0), p, true);
    CHECK(fwd.left.enable);
    CHECK(fwd.right.enable);
    CHECK(fwd.left.rpwm_duty > 0);
    CHECK(fwd.right.rpwm_duty > 0);
    CHECK_EQ(0, fwd.left.lpwm_duty);
    CHECK_EQ(0, fwd.right.lpwm_duty);

    DriveOutput pivot = drive_pipeline(snap(0, 512, 0), p, true);
    CHECK(pivot.left.rpwm_duty > 0);   // left forward
    CHECK_EQ(0, pivot.left.lpwm_duty);
    CHECK(pivot.right.lpwm_duty > 0);  // right reverse
    CHECK_EQ(0, pivot.right.rpwm_duty);

    // ARMED gate holds across the whole path
    DriveOutput disarmed = drive_pipeline(snap(512, 0, 0), p, false);
    CHECK(!disarmed.left.enable);
    CHECK(!disarmed.right.enable);
    CHECK_EQ(0, disarmed.left.rpwm_duty);
    CHECK_EQ(0, disarmed.left.lpwm_duty);
    CHECK_EQ(0, disarmed.right.rpwm_duty);
    CHECK_EQ(0, disarmed.right.lpwm_duty);
}

int main() {
    test_mixer();
    test_motor_output();
    test_pipeline();
    std::printf("\n%d checks, %d failed\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
