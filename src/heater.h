#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <optional>
#include <memory>
#include <TaskSchedulerDeclarations.h>
#include <forward_list>
#include <unordered_map>

#include "configuration.h"
#include "ambient/ambient.h"

namespace APB {

class Heater;
namespace Heaters {
    using Array = std::array<APB::Heater, APB_HEATERS_SIZE>;
    extern Array &Instance;
}


class Heater {
public:
    using GetTargetTemperature = std::function<std::optional<float>()>;
    Heater();
    ~Heater();
    enum Mode { off, fixed, target_temperature, dewpoint };
    void setup(uint8_t index, Scheduler &scheduler);
    
    float duty() const;
    void setDuty(float duty);
    bool setTemperature(float targetTemperature, float maxDuty=1);
    bool setDewpoint(float offset, float maxDuty=1);
    std::optional<float> temperature() const;
    std::optional<float> targetTemperature() const;
    std::optional<float> dewpointOffset() const;
    bool active() const;

    Mode mode() const;
    static std::forward_list<String> validModes();
    static Mode modeFromString(const String &mode);
    const String modeAsString() const;
    uint8_t index() const;
private:
    struct Private;
    friend struct Private;
    std::shared_ptr<Private> d;
};
}



#endif