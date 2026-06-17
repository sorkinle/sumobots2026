#pragma once
#include <Arduino.h>

#include "drive_logic.h"

// Device-only BTS7960 wrapper for ONE robot side (its two motors are wired in
// parallel into a single driver). Owns the RPWM/LPWM/EN GPIOs and is the only
// place that writes motor hardware. The decision of what to drive lives in the
// hardware-independent motor_output_for(); this class just applies the result.
class MotorPair {
public:
    void begin(int rpwm_gpio, int lpwm_gpio, int en_gpio,
               uint8_t ledc_ch_r, uint8_t ledc_ch_l);
    void apply(const MotorSignals& s);

private:
    int rpwm_ = -1, lpwm_ = -1, en_ = -1;
    uint8_t ch_r_ = 0, ch_l_ = 0;
};
