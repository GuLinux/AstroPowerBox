#pragma once
#include <unistd.h>
#include <array>
#include <configuration.h>
#include <list>
#include <optional>
#include <ArduinoJson.h>

#include "ambient/ambient.h"
#include "heater.h"

namespace APB {
class History {
public:
    struct Entry {
        struct Heater {
            int16_t temperatureHundredth;
            uint8_t duty;
            void set(const APB::Heater &heater);
            float getTemperature() const { return static_cast<float>(temperatureHundredth) / 100.0; }
            float getDuty() const { return static_cast<float>(duty); }
        };
        uint32_t secondsFromBoot;
        void setAmbient(const std::optional<Ambient::Reading> &reading);

        int16_t ambientTemperatureHundredth;
        int16_t ambientHumidityHundredth;

        float getAmbientTemperature() const { return static_cast<float>(ambientTemperatureHundredth) / 100.0; }
        float getAmbientHumidity() const { return static_cast<float>(ambientHumidityHundredth) / 100.0; }
        float getDewpoint() const { return Ambient::calculateDewpoint(getAmbientTemperature(), getAmbientHumidity()); }
        std::array<Heater, APB_HEATERS_TEMP_SENSORS> heaters;

        void populate(JsonObject object);
    private:
        void setNullableFloat(JsonObject object, const char *field, float value, float minValue=-50);
    };
    typedef std::list<Entry> Entries;

    void setMaxSize(uint16_t maxSize) { this->maxSize = maxSize; }

    void add(const Entry &entry);
    Entries entries() const { return _entries; }
private:
    Entries _entries;
    uint16_t maxSize = 500;
};
extern History HistoryInstance;
}
