#include "motor_pair.h"

namespace {
constexpr uint32_t kPwmFreqHz = 20000;  // 20 kHz, above the audible band
constexpr uint8_t  kPwmBits   = 10;     // 0..1023, matches DriveParams::max_power
}  // namespace

void MotorPair::begin(int rpwm_gpio, int lpwm_gpio, int en_gpio,
                      uint8_t ledc_ch_r, uint8_t ledc_ch_l) {
    rpwm_ = rpwm_gpio;
    lpwm_ = lpwm_gpio;
    en_   = en_gpio;
    ch_r_ = ledc_ch_r;
    ch_l_ = ledc_ch_l;

    pinMode(en_, OUTPUT);
    digitalWrite(en_, LOW);  // start disabled (motors off)

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    // ESP32 Arduino core 3.x: pin-based LEDC API.
    ledcAttach(rpwm_, kPwmFreqHz, kPwmBits);
    ledcAttach(lpwm_, kPwmFreqHz, kPwmBits);
    ledcWrite(rpwm_, 0);
    ledcWrite(lpwm_, 0);
#else
    // ESP32 Arduino core 2.x: channel-based LEDC API.
    ledcSetup(ch_r_, kPwmFreqHz, kPwmBits);
    ledcSetup(ch_l_, kPwmFreqHz, kPwmBits);
    ledcAttachPin(rpwm_, ch_r_);
    ledcAttachPin(lpwm_, ch_l_);
    ledcWrite(ch_r_, 0);
    ledcWrite(ch_l_, 0);
#endif
}

void MotorPair::apply(const MotorSignals& s) {
    digitalWrite(en_, s.enable ? HIGH : LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(rpwm_, s.rpwm_duty);
    ledcWrite(lpwm_, s.lpwm_duty);
#else
    ledcWrite(ch_r_, s.rpwm_duty);
    ledcWrite(ch_l_, s.lpwm_duty);
#endif
}
