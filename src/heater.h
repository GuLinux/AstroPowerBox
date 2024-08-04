#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <optional>
#include <enum.h>
#include <memory>
#include <TaskSchedulerDeclarations.h>

#include "configuration.h"
#include "ambient/ambient.h"

namespace APB {
BETTER_ENUM(Heater_Mode, uint8_t, off, fixed, target_temperature, dewpoint)

class Heater {
public:
    using GetTargetTemperature = std::function<std::optional<float>()>;
    Heater();
    ~Heater();
    using Mode = Heater_Mode;
    void setup(uint8_t index, Scheduler &scheduler, Ambient *ambient);
    float duty() const;
    void setDuty(float duty);
    bool setTemperature(float targetTemperature, float maxDuty=1);
    bool setDewpoint(float offset, float maxDuty=1);
    std::optional<float> temperature() const;
    std::optional<float> targetTemperature() const;
    std::optional<float> dewpointOffset() const;
    bool active() const;

    Mode mode() const;
    uint8_t index() const;
private:
    struct Private;
    friend struct Private;
    std::shared_ptr<Private> d;
};
using Heaters = std::array<APB::Heater, APB_HEATERS_SIZE>;
}



#endif