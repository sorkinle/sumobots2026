#include "drive_logic.h"

namespace {

// |v| < deadband => 0, otherwise pass through.
int16_t apply_deadband(int16_t v, uint16_t deadband) {
    const int32_t db = deadband;
    return (v > -db && v < db) ? 0 : v;
}

// Map a stick axis (half-range 512) onto the PWM range [-max_power, +max_power].
int32_t scale_axis(int16_t v, uint16_t max_power) {
    return (int32_t)v * (int32_t)max_power / 512;
}

int16_t clamp_to_power(int32_t v, uint16_t max_power) {
    const int32_t hi = max_power;
    if (v > hi) return (int16_t)hi;
    if (v < -hi) return (int16_t)(-hi);
    return (int16_t)v;
}

}  // namespace

DriveCommand mixer_compute(const ControllerSnapshot& s, const DriveParams& p) {
    DriveCommand cmd;
    if (p.mapping_mode == MAPPING_TANK) {
        cmd.left  = clamp_to_power(scale_axis(apply_deadband(s.ly, p.deadband), p.max_power), p.max_power);
        cmd.right = clamp_to_power(scale_axis(apply_deadband(s.ry, p.deadband), p.max_power), p.max_power);
    } else {  // arcade: ly = throttle, rx = steer
        const int32_t fwd   = scale_axis(apply_deadband(s.ly, p.deadband), p.max_power);
        const int32_t steer = scale_axis(apply_deadband(s.rx, p.deadband), p.max_power);
        cmd.left  = clamp_to_power(fwd + steer, p.max_power);
        cmd.right = clamp_to_power(fwd - steer, p.max_power);
    }
    return cmd;
}

MotorSignals motor_output_for(int16_t signed_pwm, bool armed) {
    MotorSignals out;
    if (!armed) {
        return out;  // ARMED gate: both duties 0, enable false
    }
    out.enable = true;
    if (signed_pwm >= 0) {
        out.rpwm_duty = (uint16_t)signed_pwm;
    } else {
        out.lpwm_duty = (uint16_t)(-signed_pwm);
    }
    return out;
}

DriveOutput drive_pipeline(const ControllerSnapshot& s, const DriveParams& p, bool armed) {
    const DriveCommand cmd = mixer_compute(s, p);
    DriveOutput out;
    out.left  = motor_output_for(cmd.left, armed);
    out.right = motor_output_for(cmd.right, armed);
    return out;
}
