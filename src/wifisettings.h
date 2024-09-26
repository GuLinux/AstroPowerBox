#ifndef GULINUX_WIFI_SETTINGS
#define GULINUX_WIFI_SETTINGS

#include <array>
#include <Preferences.h>
#include <WString.h>
#include <FS.h>

#ifndef WIFIMANAGER_MAX_ESSID_PSK_SIZE
#define WIFIMANAGER_MAX_ESSID_PSK_SIZE 256
#endif

#ifndef WIFIMANAGER_MAX_STATIONS
#define WIFIMANAGER_MAX_STATIONS 5
#endif

namespace GuLinux {
class WiFiSettings {
public:
    struct WiFiStation {
        char essid[WIFIMANAGER_MAX_ESSID_PSK_SIZE] = {0};
        char psk[WIFIMANAGER_MAX_ESSID_PSK_SIZE] = {0};
        operator bool() const { return valid(); }
        bool valid() const { return !empty(); }
        bool empty() const;
        bool open() const;
    };
    WiFiSettings(Preferences &preferences, FS &fs, const char *defaultHostname="ESP32-", bool appendMacSuffix=true);
    void setup();
    void load();
    void loadDefaults();
    void save();

    void setAPConfiguration(const char *essid, const char *psk);
    void setStationConfiguration(uint8_t index, const char *essid, const char *psk);
    const char *hostname() const;
    WiFiStation apConfiguration() const { return _apConfiguration; }
    WiFiStation station(uint8_t index) const { return _stations[index]; }
    bool hasStation(const String &essid) const;
    bool hasValidStations() const;
    std::array<WiFiStation, WIFIMANAGER_MAX_STATIONS> stations() const { return _stations; }
private:
    Preferences &preferences;
    FS &fs;
    const char *defaultHostname;
    bool appendMacSuffix;
    std::array<WiFiStation, WIFIMANAGER_MAX_STATIONS> _stations;
    WiFiStation _apConfiguration;
    
    void loadDefaultStations();
    
};
}
#endif
