#ifndef APB_SETTINGS_H
#define APB_SETTINGS_H
#include <array>
#include "configuration.h"
#include <WString.h>
#include "wifisettings.h"


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
private:
    Preferences prefs;
    GuLinux::WiFiSettings wifiSettings;
    float _statusLedDuty;
    void loadDefaults();
};
}
#endif
