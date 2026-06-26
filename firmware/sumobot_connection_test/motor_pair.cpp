#include "motor_pair.h"

namespace {
constexpr uint32_t kPwmFreqHz = 20000;  // 20 kHz — above audible range
constexpr uint8_t  kPwmBits   = 10;     // 0..1023, matches DriveParams::max_power
}  // namespace

void MotorPair::begin(int m1_fwd_gpio, int m1_rev_gpio,
                      int m2_fwd_gpio, int m2_rev_gpio,
                      uint8_t ch0, uint8_t ch1, uint8_t ch2, uint8_t ch3) {
    m1_fwd_ = m1_fwd_gpio;
    m1_rev_ = m1_rev_gpio;
    m2_fwd_ = m2_fwd_gpio;
    m2_rev_ = m2_rev_gpio;
    ch_[0] = ch0; ch_[1] = ch1; ch_[2] = ch2; ch_[3] = ch3;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(m1_fwd_, kPwmFreqHz, kPwmBits);
    ledcAttach(m1_rev_, kPwmFreqHz, kPwmBits);
    ledcAttach(m2_fwd_, kPwmFreqHz, kPwmBits);
    ledcAttach(m2_rev_, kPwmFreqHz, kPwmBits);
    ledcWrite(m1_fwd_, 0);
    ledcWrite(m1_rev_, 0);
    ledcWrite(m2_fwd_, 0);
    ledcWrite(m2_rev_, 0);
#else
    ledcSetup(ch_[0], kPwmFreqHz, kPwmBits);
    ledcSetup(ch_[1], kPwmFreqHz, kPwmBits);
    ledcSetup(ch_[2], kPwmFreqHz, kPwmBits);
    ledcSetup(ch_[3], kPwmFreqHz, kPwmBits);
    ledcAttachPin(m1_fwd_, ch_[0]);
    ledcAttachPin(m1_rev_, ch_[1]);
    ledcAttachPin(m2_fwd_, ch_[2]);
    ledcAttachPin(m2_rev_, ch_[3]);
    ledcWrite(ch_[0], 0);
    ledcWrite(ch_[1], 0);
    ledcWrite(ch_[2], 0);
    ledcWrite(ch_[3], 0);
#endif
}

void MotorPair::apply(const MotorSignals& s) {
    // Both motors on this side get the same signal
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(m1_fwd_, s.rpwm_duty);
    ledcWrite(m1_rev_, s.lpwm_duty);
    ledcWrite(m2_fwd_, s.rpwm_duty);
    ledcWrite(m2_rev_, s.lpwm_duty);
#else
    ledcWrite(ch_[0], s.rpwm_duty);
    ledcWrite(ch_[1], s.lpwm_duty);
    ledcWrite(ch_[2], s.rpwm_duty);
    ledcWrite(ch_[3], s.lpwm_duty);
#endif
}
