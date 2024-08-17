#ifndef APB_SETTINGS_H
#define APB_SETTINGS_H
#include <array>
#include "configuration.h"
#include <WString.h>

#define APB_MAX_ESSID_PSK_SIZE 256

namespace APB {
class Settings {
public:
    struct WiFiStation {
        char essid[APB_MAX_ESSID_PSK_SIZE] = {0};
        char psk[APB_MAX_ESSID_PSK_SIZE] = {0};
        operator bool() const { return valid(); }
        bool valid() const { return !empty(); }
        bool empty() const;
        bool open() const;
    };
    Settings();
    void setup();
    void load();
    void save();

    void setAPConfiguration(const char *essid, const char *psk);
    void setStationConfiguration(uint8_t index, const char *essid, const char *psk);
    WiFiStation apConfiguration() const { return _apConfiguration; }
    WiFiStation station(uint8_t index) const { return _stations[index]; }
    bool hasStation(const String &essid) const;
    bool hasValidStations() const;
    float statusLedDuty() const { return _statusLedDuty; };
    void setStatusLedDuty(float duty) { _statusLedDuty = duty; }
    std::array<WiFiStation, APB_MAX_STATIONS> stations() const { return _stations; }
private:
    std::array<WiFiStation, APB_MAX_STATIONS> _stations;
    WiFiStation _apConfiguration;
    float _statusLedDuty;
    void loadDefaults();
    void loadDefaultStations();
};
}
#endif
