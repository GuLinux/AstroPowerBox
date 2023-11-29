#ifndef APB_SETTINGS_H
#define APB_SETTINGS_H
#include <logger.h>
#include <array>
#include "configuration.h"

#define APB_MAX_ESSID_PSK_SIZE 256

namespace APB {
class Settings {
public:
    struct WiFiStation {
        char essid[APB_MAX_ESSID_PSK_SIZE] = {0};
        char psk[APB_MAX_ESSID_PSK_SIZE] = {0};
        operator bool() const;
        bool empty() const;
        bool open() const;
    };
    Settings(logging::Logger &logger);
    void setup();
    void load();
    void save();

    void setAPConfiguration(const char *essid, const char *psk);
    void setStationConfiguration(uint8_t index, const char *essid, const char *psk);
    WiFiStation apConfiguration() const { return _apConfiguration; }
    WiFiStation station(uint8_t index) const { return stations[index]; }
    bool hasValidStations() const;


private:
    logging::Logger &logger;
    std::array<WiFiStation, APB_MAX_STATIONS> stations;
    WiFiStation _apConfiguration;
};
}
#endif
