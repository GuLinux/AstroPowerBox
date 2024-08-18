#pragma once
#include <unistd.h>
#include <array>
#include <configuration.h>
#include <list>
#include <optional>
#include <ArduinoJson.h>
#include <memory>

#include "ambient/ambient.h"
#include "powermonitor.h"
#include "heater.h"
#include "utils.h"

#define HISTORY_ENTRY_SIZE 256

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
        void setPower(const PowerMonitor::Status &powerStatus);

        int16_t ambientTemperatureHundredth;
        int16_t ambientHumidityHundredth;
        int16_t busVoltageHundreth;
        int16_t currentHundreth;

        float getAmbientTemperature() const { return static_cast<float>(ambientTemperatureHundredth) / 100.0; }
        float getAmbientHumidity() const { return static_cast<float>(ambientHumidityHundredth) / 100.0; }
        float getDewpoint() const { return Ambient::calculateDewpoint(getAmbientTemperature(), getAmbientHumidity()); }

        float getBusVoltage() const { return static_cast<float>(busVoltageHundreth) / 100.0; }
        float getCurrent() const { return static_cast<float>(currentHundreth) / 100.0; }
        float getPower() const { return getCurrent() * getBusVoltage(); }
        std::array<Heater, APB_HEATERS_TEMP_SENSORS> heaters;

        void populate(JsonObject object);
        
    private:
        void setNullableFloat(JsonObject object, const char *field, float value, float minValue=-50);
    };

    typedef std::list<Entry> Entries;
    
    class JsonSerialiser {
    public:
        JsonSerialiser(History &history);
        int write(uint8_t *buffer, size_t maxLen, size_t index);
    private:
        History &history;
        bool headerCreated = false;
        bool footerCreated = false;
        bool firstEntrySent = false;
        Entries::iterator it;
        StaticJsonDocument<512> jsonDocument;
        size_t currentIndex = 0;
        std::unique_ptr<OverflowPrint> overflowPrint;
    };

    void setMaxSize(uint16_t maxSize) { this->maxSize = maxSize; }

    void add(const Entry &entry);
    Entries entries() const { return _entries; }

    size_t jsonSize() const { return _entries.size() * HISTORY_ENTRY_SIZE; }
private:
    Entries _entries;
    uint16_t maxSize = 300;
    bool lockInserts = false;
};
extern History HistoryInstance;
}
