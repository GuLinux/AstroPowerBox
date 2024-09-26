#include "wifisettings.h"
#include <WiFi.h>
#include <ArduinoJson.h>

#define WIFIMANAGER_KEY_AP_ESSID "ap_essid"
#define WIFIMANAGER_KEY_AP_PSK "ap_psk"

#define WIFIMANAGER_KEY_STATION_X_ESSID "station_%d_essid"
#define WIFIMANAGER_KEY_STATION_X_PSK "station_%d_psk"

#include <functional>
using namespace std::placeholders;

void runOnFormatKey(const char *format, uint16_t index, std::function<void(const char *)> apply) {
    char key[256];
    sprintf(key, format, index);
    apply(key);
}


GuLinux::WiFiSettings::WiFiSettings(Preferences &preferences, FS &fs, const char *defaultHostname, bool appendMacSuffix)
    : preferences{preferences}, fs{fs}, defaultHostname{defaultHostname}, appendMacSuffix{appendMacSuffix} {
}

void GuLinux::WiFiSettings::setup() {
    load();
}


bool GuLinux::WiFiSettings::WiFiStation::empty() const {
    return strlen(essid) == 0;
}

bool GuLinux::WiFiSettings::WiFiStation::open() const {
    return strlen(psk) == 0;
}

void GuLinux::WiFiSettings::load() {
    size_t apSSIDChars = preferences.getString(WIFIMANAGER_KEY_AP_ESSID, _apConfiguration.essid, WIFIMANAGER_MAX_ESSID_PSK_SIZE);
    // Log.traceln(LOG_SCOPE "%s characters: %d", WIFIMANAGER_KEY_AP_ESSID, apSSIDChars);
    if(apSSIDChars > 0 && _apConfiguration ) {
        preferences.getString(WIFIMANAGER_KEY_AP_PSK, _apConfiguration.psk, WIFIMANAGER_MAX_ESSID_PSK_SIZE);
        // Log.traceln(LOG_SCOPE "Loaded AP Settings: essid=`%s`, psk=`%s`", _apConfiguration.essid, _apConfiguration.psk);
    } else {
        loadDefaults();
    }
    for(uint8_t i=0; i<WIFIMANAGER_MAX_STATIONS; i++) {
        runOnFormatKey(WIFIMANAGER_KEY_STATION_X_ESSID, i, [this, i](const char *key) { preferences.getString(key, _stations[i].essid, WIFIMANAGER_MAX_ESSID_PSK_SIZE); });
        runOnFormatKey(WIFIMANAGER_KEY_STATION_X_PSK, i, [this, i](const char *key) { preferences.getString(key, _stations[i].psk, WIFIMANAGER_MAX_ESSID_PSK_SIZE); });
        // Log.traceln(LOG_SCOPE "Station %d: essid=`%s`, psk=`%s`", i, _stations[i].essid, _stations[i].psk);
    }

    if(std::none_of(_stations.begin(), _stations.end(), std::bind(&WiFiStation::valid, _1))) {
        loadDefaultStations();
    }

}

void GuLinux::WiFiSettings::loadDefaults() {
    if(!appendMacSuffix) {
        sprintf(_apConfiguration.essid, defaultHostname);
    } else {
        String mac = WiFi.macAddress();
        // Log.traceln(LOG_SCOPE "Found mac address: `%s`", mac.c_str());
        mac.replace(F(":"), F(""));
        mac = mac.substring(6);
        sprintf(_apConfiguration.essid, "%s-%s", defaultHostname, mac.c_str());
    }
    memset(_apConfiguration.psk, 0, WIFIMANAGER_MAX_ESSID_PSK_SIZE);
    // Log.traceln(LOG_SCOPE "Using default ESSID: `%s`", _apConfiguration.essid);
    loadDefaultStations();
}

void GuLinux::WiFiSettings::loadDefaultStations() {
    if(fs.exists("/wifi.json")) {
        fs::File wifiJson = fs.open("/wifi.json");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, wifiJson);
        if (error) {
            // Log.warningln(F("Failed to read file \"/wifi.json\", using empty wifi configuration"));
            wifiJson.close();
            return;
        }
        wifiJson.close();
        JsonArray stations = doc.as<JsonArray>();
        // Log.infoln("Found valid /wifi.json settings file, loading default wifi settings with %d stations", stations.size());
        for(int i=0; i<stations.size() && i<WIFIMANAGER_MAX_STATIONS; i++) {
            String ssid = stations[i]["ssid"];
            String psk = stations[i]["psk"];
            // Log.infoln("Adding station: %s", ssid);
            _stations[i] = WiFiStation{};
            strcpy(_stations[i].essid, ssid.c_str());
            strcpy(_stations[i].psk, psk.c_str());
            
        }
        save();
    }
}

void GuLinux::WiFiSettings::save() {
    // Log.traceln(LOG_SCOPE "Saving APB Settings");
    preferences.putString(WIFIMANAGER_KEY_AP_ESSID, _apConfiguration.essid);
    preferences.putString(WIFIMANAGER_KEY_AP_PSK, _apConfiguration.psk);

    for(uint8_t i=0; i<WIFIMANAGER_MAX_STATIONS; i++) {
        runOnFormatKey(WIFIMANAGER_KEY_STATION_X_ESSID, i, [this, i](const char *key) { preferences.putString(key, _stations[i].essid); });
        runOnFormatKey(WIFIMANAGER_KEY_STATION_X_PSK, i, [this, i](const char *key) { preferences.putString(key, _stations[i].psk); });
    }
    // Log.infoln(LOG_SCOPE "Preferences saved");
}


void GuLinux::WiFiSettings::setAPConfiguration(const char *essid, const char *psk) {
    strcpy(_apConfiguration.essid, essid);
    strcpy(_apConfiguration.psk, psk);
}

void GuLinux::WiFiSettings::setStationConfiguration(uint8_t index, const char *essid, const char *psk) {
    strcpy(_stations[index].essid, essid);
    strcpy(_stations[index].psk, psk);
}

const char *GuLinux::WiFiSettings::hostname() const {
    return _apConfiguration.essid;
}

bool GuLinux::WiFiSettings::hasStation(const String &essid) const {
    return std::any_of(
        std::begin(_stations),
        std::end(_stations),
        [&essid](const WiFiStation &station){
            return essid == station.essid;
        }
    );
}

bool GuLinux::WiFiSettings::hasValidStations() const {
    return std::any_of(_stations.begin(), _stations.end(), std::bind(&GuLinux::WiFiSettings::WiFiStation::valid, _1));
}
