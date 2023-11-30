#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <logger.h>
#include <optional>
#include <enum.h>
#include <TaskSchedulerDeclarations.h>

#include "configuration.h"
#include <memory>
namespace APB {
BETTER_ENUM(Heater_Mode, uint8_t, Off, FixedPWM, SetTemperature)

class Heater {
public:
    Heater();
    ~Heater();
    using Mode = Heater_Mode;
    void setup(logging::Logger &logger, uint8_t index, Scheduler &scheduler);
    float pwm() const;
    void pwm(float duty);
    bool temperature(float temperature, float maxDuty=1);
    std::optional<float> temperature() const;

    Mode mode() const;
    uint8_t index() const;
#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
    void setSimulation(const std::optional<float> &temperature);
#endif
private:
    struct Private;
    friend struct Private;
    std::shared_ptr<Private> d;
};
using Heaters = std::array<APB::Heater, APB_HEATERS_SIZE>;
}



#endif