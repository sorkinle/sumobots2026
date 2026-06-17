#pragma once
#include <cstdint>

// Hardware-independent drive logic for the Controller -> ESP32 -> motor-driver
// path. No Arduino headers here on purpose: the .ino includes this to run on the
// ESP32, and the host test (firmware/test) compiles it on a PC for fast TDD.

// ---- Controller input (filled on-device from Bluepad32) -------------------
struct ControllerSnapshot {
    int16_t  lx = 0, ly = 0, rx = 0, ry = 0;  // sticks, -512..+511 (+ly=forward, +rx=steer right)
    int16_t  lt = 0, rt = 0;                  // triggers, 0..1023
    uint16_t buttons = 0;                     // see btn:: below
    uint32_t last_event_ms = 0;
    bool     valid = false;
};

namespace btn {
constexpr uint16_t A = 1 << 0, B = 1 << 1, X = 1 << 2, Y = 1 << 3,
                   LB = 1 << 4, RB = 1 << 5, MENU = 1 << 6;
}

// ---- Drive mixing ---------------------------------------------------------
enum MappingMode : uint8_t { MAPPING_ARCADE = 0, MAPPING_TANK = 1 };

struct DriveParams {
    uint8_t  mapping_mode = MAPPING_ARCADE;
    uint16_t deadband     = 40;    // raw stick counts; |axis| < deadband => 0
    uint16_t max_power    = 700;   // output clamp, 0..1023 (10-bit PWM)
};

struct DriveCommand {
    int16_t left = 0, right = 0;   // signed PWM, [-max_power, +max_power]
};

// Pure: controller input -> per-side signed PWM command.
DriveCommand mixer_compute(const ControllerSnapshot& s, const DriveParams& p);

// ---- Motor-driver signals (one BTS7960 side) ------------------------------
struct MotorSignals {
    uint16_t rpwm_duty = 0;  // forward duty (cmd >= 0)
    uint16_t lpwm_duty = 0;  // reverse duty (cmd <  0)
    bool     enable    = false;
};

// Pure: signed PWM command + armed flag -> driver signals.
// SAFETY GATE (FR-010 / NFR-003): when !armed, both duties are 0 and enable is
// false, regardless of command. This is the single ARMED gate for motor output.
MotorSignals motor_output_for(int16_t signed_pwm, bool armed);

// ---- Whole path in one pure call -----------------------------------------
struct DriveOutput {
    MotorSignals left, right;
};

// Controller snapshot -> mix -> per-side ARMED-gated driver signals. The .ino
// just pushes the result onto GPIO/LEDC via MotorPair.
DriveOutput drive_pipeline(const ControllerSnapshot& s, const DriveParams& p, bool armed);
