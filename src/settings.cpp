#include "settings.h"
#include <Preferences.h>
#include <WiFi.h>

#define APB_PREFS_VERSION 1
#define APB_KEY_VERSION "version"
#define APB_KEY_AP_ESSID "ap_essid"
#define APB_KEY_AP_PSK "ap_psk"

#define APB_KEY_STATION_X_ESSID "station_%d_essid"
#define APB_KEY_STATION_X_PSK "station_%d_psk"


#define LOG_SCOPE F("APB::Configuration")

using namespace std::placeholders;

namespace {
    Preferences prefs;
}

bool APB::Settings::WiFiStation::empty() const {
    return strlen(essid) == 0;
}

bool APB::Settings::WiFiStation::open() const {
    return strlen(psk) == 0;
}

APB::Settings::Settings(logging::Logger &logger) : logger(logger) {

}

void APB::Settings::setup() {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Setup");
    prefs.begin("APB");
    load();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Setup finished");
}

void runOnFormatKey(const char *format, uint16_t index, std::function<void(const char *)> apply) {
    char key[256];
    sprintf(key, format, index);
    apply(key);
}

void APB::Settings::load() {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Loading APB Settings");
    uint16_t storedVersion = prefs.getUShort(APB_KEY_VERSION);
    size_t apSSIDChars = prefs.getString(APB_KEY_AP_ESSID, _apConfiguration.essid, APB_MAX_ESSID_PSK_SIZE);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "%s characters: %d", APB_KEY_AP_ESSID, apSSIDChars);
    if(apSSIDChars > 0 && _apConfiguration ) {
        prefs.getString(APB_KEY_AP_PSK, _apConfiguration.psk, APB_MAX_ESSID_PSK_SIZE);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, 
            "Loaded AP Settings: essid=`%s`, psk=`%s`", _apConfiguration.essid, _apConfiguration.psk);
    } else {
        String mac = WiFi.macAddress();
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Found mac address: `%s`", mac.c_str());
        mac.replace(F(":"), F(""));
        mac = mac.substring(6);
        sprintf(_apConfiguration.essid, "AstroPowerBox-%s", mac.c_str());
        memset(_apConfiguration.psk, 0, APB_MAX_ESSID_PSK_SIZE);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Using default ESSID: `%s`", _apConfiguration.essid);
    }
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        runOnFormatKey(APB_KEY_STATION_X_ESSID, i, [this, i](const char *key) { prefs.getString(key, stations[i].essid, APB_MAX_ESSID_PSK_SIZE); });
        runOnFormatKey(APB_KEY_STATION_X_PSK, i, [this, i](const char *key) { prefs.getString(key, stations[i].psk, APB_MAX_ESSID_PSK_SIZE); });
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Station %d: essid=`%s`, psk=`%s`", i, stations[i].essid, stations[i].psk);
    }
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Preferences loaded");
}

void APB::Settings::save() {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Saving APB Settings");
    prefs.putUShort(APB_KEY_VERSION, APB_PREFS_VERSION);
    prefs.putString(APB_KEY_AP_ESSID, _apConfiguration.essid);
    prefs.putString(APB_KEY_AP_PSK, _apConfiguration.psk);

    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        runOnFormatKey(APB_KEY_STATION_X_ESSID, i, [this, i](const char *key) { prefs.putString(key, stations[i].essid); });
        runOnFormatKey(APB_KEY_STATION_X_PSK, i, [this, i](const char *key) { prefs.putString(key, stations[i].psk); });
    }
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Preferences saved");
}

void APB::Settings::setAPConfiguration(const char *essid, const char *psk) {
    strcpy(_apConfiguration.essid, essid);
    strcpy(_apConfiguration.psk, psk);
}

void APB::Settings::setStationConfiguration(uint8_t index, const char *essid, const char *psk) {
    strcpy(stations[index].essid, essid);
    strcpy(stations[index].psk, psk);
}

bool APB::Settings::hasValidStations() const {
    return std::any_of(stations.begin(), stations.end(), std::bind(&APB::Settings::WiFiStation::valid, _1));
}
