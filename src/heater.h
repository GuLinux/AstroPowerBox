#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <logger.h>
#include <optional>
#include <enum.h>

#include "configuration.h"

namespace APB {

BETTER_ENUM(Heater_Mode, uint8_t, Off, FixedPWM, SetTemperature)
class Heater {
public:
    using Mode = Heater_Mode;
    void setup(logging::Logger &logger, uint8_t index);
    float pwm() const;
    void pwm(float duty);
    bool temperature(float temperature, float maxDuty=1);
    std::optional<float> temperature() const;

    Mode mode() const;
    uint8_t index() const { return _index; }
    void tick();
private:
    logging::Logger *logger;
    uint8_t _index;
    char log_scope[20];

};
using Heaters = std::array<APB::Heater, APB_HEATERS>;
}



#endif