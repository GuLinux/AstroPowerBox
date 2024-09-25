#include "wifimanager.h"
#include <ArduinoLog.h>
#include <WiFi.h>

#define LOG_SCOPE "WiFiManager:"

using namespace std::placeholders;

APB::WiFiManager &APB::WiFiManager::Instance = *new APB::WiFiManager();

APB::WiFiManager::WiFiManager() : 
    _status{Status::Idle},
    rescanWiFiTask{3'000, TASK_ONCE, std::bind(&WiFiManager::startScanning, this)} {
    WiFi.onEvent(std::bind(&APB::WiFiManager::onEvent, this, _1, _2));
}

void APB::WiFiManager::onScanDone(const wifi_event_sta_scan_done_t &scan_done) {
    bool success = scan_done.status == 0;
    Log.traceln(LOG_SCOPE "[EVENT] Scan done: success=%d, APs found: %d, scheduleReconnect=%d, connectionFailed=%d", success, scan_done.number, scheduleReconnect, connectionFailed);
    if(success) {
        for(uint8_t i=0; i<scan_done.number; i++) {
            Log.traceln(LOG_SCOPE "[EVENT] AP[%d]: ESSID=%s, channel: %d, RSSI: %d", i, WiFi.SSID(i), WiFi.channel(i), WiFi.RSSI(i));
        }
        if(!connectionFailed) {
            return;
        }
        for(uint8_t i=0; i<scan_done.number; i++) {
            if(Settings::Instance.hasStation(WiFi.SSID(i))) {
                Log.infoln(LOG_SCOPE "[EVENT] Found at least one AP from configuration, scheduling reconnection");
                scheduleReconnect = true;
                return;
            } else {
                Log.infoln(LOG_SCOPE "[EVENT] No known APs found, scheduling rescan");
                rescanWiFiTask.enable();
            }
        }
    }
}

void APB::WiFiManager::startScanning() {
    WiFi.scanNetworks(true);
}

void APB::WiFiManager::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch(event) {
        case(ARDUINO_EVENT_WIFI_SCAN_DONE):
            onScanDone(info.wifi_scan_done);
            break;
        case(ARDUINO_EVENT_WIFI_AP_START):
            _status = Status::AccessPoint;
            Log.infoln(LOG_SCOPE "[EVENT] Access Point started");
            break;
        case(ARDUINO_EVENT_WIFI_STA_CONNECTED):
            Log.infoln(LOG_SCOPE "[EVENT] Connected to station `%s`, channel %d",
                reinterpret_cast<char*>(info.wifi_sta_connected.ssid),
                info.wifi_sta_connected.channel);
            _status = Status::Station;
            std::for_each(onConnectedCallbacks.begin(), onConnectedCallbacks.end(), std::bind(&OnConnectCallback::operator(), _1));
            break;
        case(ARDUINO_EVENT_WIFI_STA_DISCONNECTED):
        case(ARDUINO_EVENT_WIFI_STA_STOP):
            Log.infoln(LOG_SCOPE "[EVENT] WiFi disconnected: SSID=%s, reason=%d",
                reinterpret_cast<char*>(info.wifi_sta_disconnected.ssid),
                info.wifi_sta_disconnected.reason);
            _status = Status::Idle;
            scheduleReconnect = true;
            break;
        case(ARDUINO_EVENT_WIFI_AP_STOP):
            Log.infoln(LOG_SCOPE "[EVENT] WiFi AP stopped");
            _status = Status::Idle;
            break;
        default:
            Log.traceln("[EVENT] Unknown event %d", event);
    }
}

void APB::WiFiManager::setup(Scheduler &scheduler) {
    Log.traceln(LOG_SCOPE "setup");
    scheduler.addTask(rescanWiFiTask);

    WiFi.setHostname(Settings::Instance.apConfiguration().essid);
    _status = Status::Connecting;
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        auto station = Settings::Instance.station(i);
        if(station) {
            Log.infoln(LOG_SCOPE "found valid station: %s", station.essid);
            wifiMulti.addAP(station.essid, station.psk);
        }
    }
    connect();
    Log.infoln(LOG_SCOPE "setup finished");
}

void APB::WiFiManager::setApMode() {
    Log.infoln(LOG_SCOPE "Starting softAP with essid=`%s`, ip address=`%s`",
            Settings::Instance.apConfiguration().essid, WiFi.softAPIP().toString().c_str());
    WiFi.softAP(Settings::Instance.apConfiguration().essid, 
        Settings::Instance.apConfiguration().open() ? nullptr : Settings::Instance.apConfiguration().psk);
}

void APB::WiFiManager::connect() {
    connectionFailed = false;
    bool hasValidStations = Settings::Instance.hasValidStations();
    if(!hasValidStations) {
        Log.warningln(LOG_SCOPE "No valid stations found");
        setApMode();
        StatusLed::Instance.noWiFiStationsFoundPattern();
        return;
    }
    WiFi.mode(WIFI_MODE_STA);
    if( wifiMulti.run() != WL_CONNECTED) {
        Log.warningln(LOG_SCOPE "Unable to connect to WiFi stations");
        StatusLed::Instance.wifiConnectionFailedPattern();
        setApMode();
        connectionFailed = true;
        rescanWiFiTask.enable();
        return;
    }
    StatusLed::Instance.okPattern();
    Log.infoln(LOG_SCOPE "Connected to WiFi `%s`, ip address: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void APB::WiFiManager::loop() {
    if(scheduleReconnect) {
        scheduleReconnect = false;
        connect();
    }
}

void APB::WiFiManager::addOnConnectedListener(const OnConnectCallback &onConnected) {
    onConnectedCallbacks.push_back(onConnected);
}

const char *APB::WiFiManager::statusAsString() const
{
    switch (_status) {
    case Status::AccessPoint:
        return "AccessPoint";
    case Status::Connecting:
        return "Connecting";
    case Status::Error:
        return "Error";
    case Status::Idle:
        return "Idle";
    case Status::Station:
        return "Station";
    default:
        return "N/A";
    }
}

String APB::WiFiManager::essid() const
{
    if(_status == +Status::Station) {
        return WiFi.SSID();
    }
    if(_status == +Status::AccessPoint) {
        return Settings::Instance.apConfiguration().essid;
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

