#ifndef APB_SETTINGS_H
#define APB_SETTINGS_H
#include <array>
#include "configuration.h"
#include <WString.h>
#include <wifisettings.h>
#include "powermonitor.h"
#include <unordered_map>
#include "pd_protocol.h"

#define APB_CONFIG_DIRECTORY "/config"

namespace APB {
class Settings {
public:
    static Settings &Instance;
    Settings();
    void setup();
    void load();
    void save();
    GuLinux::WiFiSettings &wifi() { return wifiSettings; }

    float statusLedDuty() const { return _statusLedDuty; };
    float fanDuty() const { return _fanDuty; };
    PDProtocol::Voltage pdVoltage() const { return _pdVoltage; }
    void setPdVoltage(PDProtocol::Voltage voltage) { _pdVoltage = voltage; }
    void setStatusLedDuty(float duty) { _statusLedDuty = duty; }
    void setFanDuty(float duty) { _fanDuty = duty; }
    PowerMonitor::PowerSource powerSource() const;
    void setPowerSource(PowerMonitor::PowerSource powerSource);

    static const std::unordered_map<PowerMonitor::PowerSource, const char*> PowerSourcesNames;
private:
    Preferences prefs;
    GuLinux::WiFiSettings wifiSettings;
    float _statusLedDuty;
    float _fanDuty;
    PDProtocol::Voltage _pdVoltage = PDProtocol::V12;
    PowerMonitor::PowerSource _powerSource;
    void loadDefaults();
};
}
#endif
