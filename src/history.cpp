#include "history.h"
#include <ArduinoLog.h>
#include "utils.h"

void APB::History::Entry::Heater::set(const APB::Heater &heater) {
    temperatureHundredth = static_cast<int16_t>(heater.temperature().value_or(-100.0) * 100.0);
    duty = heater.active() ? heater.duty() : 0;
}

void APB::History::Entry::setAmbient(const std::optional<Ambient::Reading> &reading) {
    const auto readingValue = reading.value_or(Ambient::Reading{-100.0, -100.0});
    ambientTemperatureHundredth = static_cast<uint16_t>(readingValue.temperature * 100.0);
    ambientHumidityHundredth = static_cast<uint16_t>(readingValue.humidity * 100.0);
}

void APB::History::Entry::populate(JsonObject object) {
    object["uptime"] = secondsFromBoot;
    setNullableFloat(object, "ambientTemperature", getAmbientTemperature());
    setNullableFloat(object, "ambientHumidity", getAmbientHumidity());
    object["ambientDewpoint"] = getDewpoint();
    for(uint8_t i=0; i<heaters.size(); i++) {
        JsonObject heaterObject = object["heaters"][i].to<JsonObject>();;
        heaterObject["duty"] = heaters[i].getDuty();
        setNullableFloat(heaterObject, "temperature", heaters[i].getTemperature());
    }
}


void APB::History::Entry::setNullableFloat(JsonObject object, const char *field, float value, float minValue) {
    if(value < minValue) {
        object[field] = (char*)0;
    } else {
        object[field] = value;
    }
}



void APB::History::add(const Entry &entry) {
    _entries.push_back(entry);
    while(_entries.size() > maxSize) {
        _entries.pop_front();
    }
}