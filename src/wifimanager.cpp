#include "wifimanager.h"
#include <ArduinoLog.h>
#include <WiFi.h>


#define LOG_SCOPE "APB::WiFiManager"

using namespace std::placeholders;

APB::WiFiManager::WiFiManager(APB::Settings &configuration)
    : configuration(configuration), _status{Status::Idle} {
    WiFi.onEvent(std::bind(&APB::WiFiManager::onEvent, this, _1, _2));
}


void APB::WiFiManager::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch(event) {
        case(ARDUINO_EVENT_WIFI_AP_START):
            _status = Status::AccessPoint;
            Log.infoln(LOG_SCOPE "[EVENT] Access Point started");
            break;
        case(ARDUINO_EVENT_WIFI_STA_CONNECTED):
            Log.infoln(LOG_SCOPE "[EVENT] Connected to station");
            _status = Status::Station;
            break;
        case(ARDUINO_EVENT_WIFI_STA_DISCONNECTED):
        case(ARDUINO_EVENT_WIFI_STA_STOP):
            Log.infoln(LOG_SCOPE "[EVENT] WiFi disconnected");
            _status = Status::Idle;
            connect();
            break;
        case(ARDUINO_EVENT_WIFI_AP_STOP):
            Log.infoln(LOG_SCOPE "[EVENT] WiFi AP stopped");
            _status = Status::Idle;
            break;
    }
}

void APB::WiFiManager::setup() {
    Log.traceln(LOG_SCOPE "setup");
    WiFi.setHostname(configuration.apConfiguration().essid);
    _status = Status::Connecting;
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        auto station = configuration.station(i);
        if(station) {
            Log.infoln(LOG_SCOPE "found valid station: %s", station.essid);
            wifiMulti.addAP(station.essid, station.psk);
        }
    }
    connect();
    Log.infoln(LOG_SCOPE "setup finished");
}

void APB::WiFiManager::connect() {
    bool hasValidStations = configuration.hasValidStations();
    if( ! hasValidStations || wifiMulti.run() != WL_CONNECTED) {
        if(!hasValidStations) {
            Log.warningln(LOG_SCOPE "No valid stations found");
        }
        Log.warningln(LOG_SCOPE "Unable to connect to WiFi stations, starting softAP with essid=`%s`, ip address=`%s`",
            configuration.apConfiguration().essid, WiFi.softAPIP().toString().c_str());
        WiFi.softAP(configuration.apConfiguration().essid, configuration.apConfiguration().open() ? nullptr : configuration.apConfiguration().psk);
    } else {
        Log.infoln(LOG_SCOPE "Connected to WiFi `%s`, ip address: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    }
}

void APB::WiFiManager::loop() {
    if(scheduleReconnect) {
        scheduleReconnect = false;
        connect();
    }
}

String APB::WiFiManager::essid() const {
    if(_status == +Status::Station) {
        return WiFi.SSID();
    }
    if(_status == +Status::AccessPoint) {
        return configuration.apConfiguration().essid;
    }
    return "N/A";
}

String APB::WiFiManager::ipAddress() const {
    if(_status == +Status::AccessPoint || _status == +Status::Station) {
        return WiFi.localIP().toString();
    }
    return "N/A"; 
}

String APB::WiFiManager::gateway() const {
    if(_status == +Status::Station) {
        return WiFi.gatewayIP().toString();
    }
    return "N/A"; 
}

