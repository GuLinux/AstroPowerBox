#ifndef APB_HEATER_H
#define APB_HEATER_H

#include <optional>
#include <memory>
#include <TaskSchedulerDeclarations.h>
#include <forward_list>
#include <unordered_map>
#include <ArduinoJson.h>

#include "configuration.h"
#include "ambient/ambient.h"

namespace APB {

class Heater;
namespace Heaters {
    using Array = std::array<APB::Heater, APB_HEATERS_SIZE>;
    extern Array &Instance;
    void toJson(JsonArray heatersStatus);
    void saveConfig();
}


class Heater {
public:
    using GetTargetTemperature = std::function<std::optional<float>()>;
    Heater();
    ~Heater();
    enum Mode { off, fixed, target_temperature, dewpoint };
    void setup(uint8_t index, Scheduler &scheduler);

    void toJson(JsonObject heaterStatus);
    
    float maxDuty() const;
    float duty() const;
    
    const char *setState(JsonObject jsonObject);

    std::optional<float> temperature() const;
    std::optional<float> targetTemperature() const;
    std::optional<float> dewpointOffset() const;
    std::optional<float> rampOffset() const;
    std::optional<float> minDuty() const;
    bool active() const;
    bool applyAtStartup() const;

    Mode mode() const;
    static std::forward_list<String> validModes();
    static Mode modeFromString(const String &mode);
    const String modeAsString() const;
    uint8_t index() const;
private:
    bool setTemperature(float targetTemperature, float maxDuty=1, float minDuty=0, float rampOffset=0);
    bool setDewpoint(float offset, float maxDuty=1, float minDuty=0, float rampOffset=0);
    void setMaxDuty(float duty);
    void loadFromJson();


    struct Private;
    friend struct Private;
    std::shared_ptr<Private> d;
};
}



#endif