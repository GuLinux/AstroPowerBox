#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <logger.h>
#include <optional>
#include <enum.h>

namespace APB {

BETTER_ENUM(Heater_Mode, uint8_t, Off, FixedPWM, SetTemperature)
class Heater {
public:
    using Mode = Heater_Mode;
    Heater(logging::Logger &logger);
    void setup();
    float pwm() const;
    void pwm(float duty);
    bool temperature(float temperature, float maxDuty=1);
    std::optional<float> temperature() const;

    Mode mode() const;
};
}

#endif