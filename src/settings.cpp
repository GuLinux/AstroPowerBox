#include "settings.h"
#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define APB_PREFS_VERSION 1
#define APB_KEY_VERSION "version"

#define APB_KEY_STATUS_LED_DUTY "status_led_duty"

#define LOG_SCOPE "APB::Configuration - "

using namespace std::placeholders;

APB::Settings &APB::Settings::Instance = *new APB::Settings();


APB::Settings::Settings() :
#ifdef APB_DEFAULT_HOSTNAME
wifiSettings{prefs, LittleFS, APB_DEFAULT_HOSTNAME, false}
#else
wifiSettings{prefs, LittleFS, "AstroPowerBox-", true}
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
    wifiSettings.load();
    Log.infoln(LOG_SCOPE "Preferences loaded");
}

void APB::Settings::loadDefaults() {
    wifiSettings.loadDefaults();
}



void APB::Settings::save() {
    Log.traceln(LOG_SCOPE "Saving APB Settings");
    prefs.putUShort(APB_KEY_VERSION, APB_PREFS_VERSION);
    wifiSettings.save();
    prefs.putFloat(APB_KEY_STATUS_LED_DUTY, _statusLedDuty);
    Log.infoln(LOG_SCOPE "Preferences saved");
}
