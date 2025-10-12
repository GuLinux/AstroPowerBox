#include "settings.h"
#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define APB_PREFS_VERSION 1
#define APB_KEY_VERSION "version"

#define APB_KEY_STATUS_LED_DUTY "status_led_duty"
#define APB_KEY_FAN_DUTY "fan_duty"
#define APB_KEY_POWER_SOURCE_TYPE "power_src_type"

#define LOG_SCOPE "APB::Configuration - "

using namespace std::placeholders;

APB::Settings &APB::Settings::Instance = *new APB::Settings();

const std::unordered_map<APB::PowerMonitor::PowerSource, const char*> APB::Settings::PowerSourcesNames = {
    {PowerMonitor::AC, "AC"},
    {PowerMonitor::LipoBattery3C, "lipo_3c"},
};



APB::Settings::Settings() :
#ifdef APB_DEFAULT_HOSTNAME
wifiSettings{prefs, LittleFS, APB_DEFAULT_HOSTNAME, false, 5, true, 3}
#else
wifiSettings{prefs, LittleFS, "AstroPowerBox-", true, 5, true, 3}
#endif
{
}

void APB::Settings::setup() {
    Log.traceln(LOG_SCOPE "Setup");
    prefs.begin("APB");
    load();

    Log.infoln(LOG_SCOPE "Setup finished");
}


void APB::Settings::load() {
    Log.traceln(LOG_SCOPE "Loading APB Settings");
    uint16_t storedVersion = prefs.getUShort(APB_KEY_VERSION);
    if(storedVersion != APB_PREFS_VERSION) {
        loadDefaults();
        return;
    }
    _statusLedDuty = prefs.getFloat(APB_KEY_STATUS_LED_DUTY, 1);
    _fanDuty = prefs.getFloat(APB_KEY_FAN_DUTY, 1.0);
    _pdVoltage = static_cast<PDProtocol::Voltage>(prefs.getUShort("pd_voltage", static_cast<uint16_t>(PDProtocol::V12)));
    _powerSource = static_cast<PowerMonitor::PowerSource>(prefs.getUShort(APB_KEY_POWER_SOURCE_TYPE, static_cast<uint16_t>(PowerMonitor::AC)));
    wifiSettings.load();
    Log.infoln(LOG_SCOPE "Preferences loaded");
}

void APB::Settings::loadDefaults() {
    wifiSettings.loadDefaults();
    _powerSource = PowerMonitor::AC;
    _statusLedDuty = 1.0;
    _fanDuty = 1.0;
    _pdVoltage = PDProtocol::V12;
}



void APB::Settings::save() {
    Log.traceln(LOG_SCOPE "Saving APB Settings");
    prefs.putUShort(APB_KEY_VERSION, APB_PREFS_VERSION);
    wifiSettings.save();
    prefs.putFloat(APB_KEY_STATUS_LED_DUTY, _statusLedDuty);
    prefs.putFloat(APB_KEY_FAN_DUTY, _fanDuty);
    prefs.putUShort(APB_KEY_POWER_SOURCE_TYPE, static_cast<uint16_t>(_powerSource));
    prefs.putUShort("pd_voltage", static_cast<uint16_t>(_pdVoltage));
    Log.infoln(LOG_SCOPE "Preferences saved");
}



APB::PowerMonitor::PowerSource APB::Settings::powerSource() const
{
    return _powerSource;
}

void APB::Settings::setPowerSource(PowerMonitor::PowerSource powerSource) {
    this->_powerSource = powerSource;
}
