#pragma once
#include <unistd.h>
#include <array>
#include <configuration.h>
#include <list>
#include <optional>
#include <ArduinoJson.h>
#include <memory>
#include <TaskSchedulerDeclarations.h>

#include "ambient/ambient.h"
#include "powermonitor.h"
#include "pwm_output.h"
#include "utils.h"

#define HISTORY_ENTRY_SIZE 256

namespace APB {

class History {
public:
    History();
    struct Entry {
        uint32_t secondsFromBoot;
        #if APB_PWM_OUTPUTS_SIZE > 0
        struct PWMOutput {
            int16_t temperatureHundredth;
            uint8_t duty;
            void set(const APB::PWMOutput &pwmOutput);
            float getTemperature() const { return static_cast<float>(temperatureHundredth) / 100.0; }
            float getDuty() const { return static_cast<float>(duty); }
        };
        std::array<PWMOutput, APB_PWM_OUTPUTS_TEMP_SENSORS> pwmOutputs;
        #endif
        
        #ifndef APB_AMBIENT_TEMPERATURE_SENSOR_NONE
        void setAmbient(const std::optional<Ambient::Reading> &reading);
        float getAmbientTemperature() const { return static_cast<float>(ambientTemperatureHundredth) / 100.0; }
        float getAmbientHumidity() const { return static_cast<float>(ambientHumidityHundredth) / 100.0; }
        float getDewpoint() const { return Ambient::calculateDewpoint(getAmbientTemperature(), getAmbientHumidity()); }
        int16_t ambientTemperatureHundredth;
        int16_t ambientHumidityHundredth;
        #endif
        void setPower(const PowerMonitor::Status &powerStatus);

        int16_t busVoltageHundreth;
        int16_t currentHundreth;

        
        float getBusVoltage() const { return static_cast<float>(busVoltageHundreth) / 100.0; }
        float getCurrent() const { return static_cast<float>(currentHundreth) / 100.0; }
        float getPower() const { return getCurrent() * getBusVoltage(); }
        

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
        JsonDocument jsonDocument;
        size_t currentIndex = 0;
        std::unique_ptr<OverflowPrint> overflowPrint;
    };

    void setup(Scheduler &scheduler);
    void setMaxSize(uint16_t maxSize) { this->maxSize = maxSize; }
    void add();

    Entries entries() const { return _entries; }

    size_t jsonSize() const { return _entries.size() * HISTORY_ENTRY_SIZE; }

    static History &Instance;
private:
    Entries _entries;
    uint16_t maxSize = 300;
    bool lockInserts = false;
};
}
