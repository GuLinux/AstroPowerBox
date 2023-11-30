#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <optional>
#include <enum.h>
#include <memory>
#include <TaskSchedulerDeclarations.h>

#include "configuration.h"

namespace APB {
BETTER_ENUM(Heater_Mode, uint8_t, Off, FixedPWM, SetTemperature)

class Heater {
public:
    using GetTargetTemperature = std::function<float()>;
    Heater();
    ~Heater();
    using Mode = Heater_Mode;
    void setup(uint8_t index, Scheduler &scheduler);
    float pwm() const;
    void setPWM(float duty);
    bool setTemperature(GetTargetTemperature getTemperature, float maxDuty=1);
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