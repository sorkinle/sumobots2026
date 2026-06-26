#pragma once
#include <Arduino.h>

#include "drive_logic.h"

// ZK-BM1 dual-channel driver wrapper for ONE robot side (two motors, each on
// its own driver channel). Owns 4 PWM GPIOs (IN1-IN4) and is the only place
// that writes motor hardware. The decision of what to drive lives in the
// hardware-independent motor_output_for(); this class just applies the result.
class MotorPair {
public:
    void begin(int m1_fwd_gpio, int m1_rev_gpio,
               int m2_fwd_gpio, int m2_rev_gpio,
               uint8_t ch0, uint8_t ch1, uint8_t ch2, uint8_t ch3);
    void apply(const MotorSignals& s);

private:
    int m1_fwd_ = -1, m1_rev_ = -1;
    int m2_fwd_ = -1, m2_rev_ = -1;
    uint8_t ch_[4] = {};
};
