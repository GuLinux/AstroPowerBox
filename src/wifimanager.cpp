#include "wifimanager.h"
#include <WiFi.h>


#define LOG_SCOPE F("APB::WiFiManager")

using namespace std::placeholders;

APB::WiFiManager::WiFiManager(APB::Settings &configuration, logging::Logger &logger)
    : configuration(configuration), logger(logger), _status{Status::Idle} {
    WiFi.onEvent(std::bind(&APB::WiFiManager::onEvent, this, _1, _2));
}


void APB::WiFiManager::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch(event) {
        case(ARDUINO_EVENT_WIFI_AP_START):
            _status = Status::AccessPoint;
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "[EVENT] Access Point started");
            break;
        case(ARDUINO_EVENT_WIFI_STA_CONNECTED):
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "[EVENT] Connected to station");
            _status = Status::Station;
            break;
        case(ARDUINO_EVENT_WIFI_STA_DISCONNECTED):
        case(ARDUINO_EVENT_WIFI_STA_STOP):
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "[EVENT] WiFi disconnected");
            _status = Status::Idle;
            connect();
            break;
        case(ARDUINO_EVENT_WIFI_AP_STOP):
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "[EVENT] WiFi AP stopped");
            _status = Status::Idle;
            break;
    }
}

void APB::WiFiManager::setup() {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "setup");
    _status = Status::Connecting;
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        auto station = configuration.station(i);
        if(station) {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "found valid station: %s", station.essid);
            wifiMulti.addAP(station.essid, station.psk);
        }
    }
    connect();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "setup finished");
}

void APB::WiFiManager::connect() {
    bool hasValidStations = configuration.hasValidStations();
    if( ! hasValidStations || wifiMulti.run() != WL_CONNECTED) {
        if(!hasValidStations) {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, LOG_SCOPE, "No valid stations found");
        }
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, LOG_SCOPE, "Unable to connect to WiFi stations, starting softAP with essid=`%s`, ip address=`%s`",
            configuration.apConfiguration().essid, WiFi.softAPIP().toString().c_str());
        WiFi.softAP(configuration.apConfiguration().essid, configuration.apConfiguration().open() ? nullptr : configuration.apConfiguration().psk);
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Connected to WiFi `%s`, ip address: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    }
}