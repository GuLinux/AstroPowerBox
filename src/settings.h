#ifndef APB_SETTINGS_H
#define APB_SETTINGS_H
#include <array>
#include "configuration.h"
#include <WString.h>
#include "wifisettings.h"
#include "powermonitor.h"
#include <unordered_map>

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
    void setStatusLedDuty(float duty) { _statusLedDuty = duty; }
    PowerMonitor::PowerSource powerSource() const;
    void setPowerSource(PowerMonitor::PowerSource powerSource);

    static const std::unordered_map<PowerMonitor::PowerSource, const char*> PowerSourcesNames;
private:
    Preferences prefs;
    GuLinux::WiFiSettings wifiSettings;
    float _statusLedDuty;
    PowerMonitor::PowerSource _powerSource;
    void loadDefaults();
};
}
#endif
