#ifndef APB_WIFIMANAGER_H
#define APB_WIFIMANAGER_H

#include <WiFiMulti.h>
#include <statusled.h>
#include <vector>
#include "settings.h"
#include <TaskSchedulerDeclarations.h>
#include "wifisettings.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

namespace APB {

class WiFiManager {
public:
    using OnConnectCallback = std::function<void()>;
    static WiFiManager &Instance;
    enum Status { Idle, Connecting, Station, AccessPoint, Error };
    WiFiManager();
    void setup(Scheduler &scheduler);
    
    void reconnect() { scheduleReconnect = true; }
    Status status() const { return _status; }
    const char *statusAsString() const;
    String essid() const;
    String ipAddress() const;
    String gateway() const;
    void loop();
    void addOnConnectedListener(const OnConnectCallback &onConnected);

    void onGetConfig(AsyncWebServerRequest *request);
    void onGetConfig(JsonObject &responseObject);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);

private:
    GuLinux::WiFiSettings *wifiSettings;
    WiFiMulti wifiMulti;
    Status _status;
    bool scheduleReconnect = false;
    bool connectionFailed = false;
    Task rescanWiFiTask;
    std::list<OnConnectCallback> onConnectedCallbacks;

    void connect();
    void setApMode();
    void onEvent(arduino_event_id_t event, arduino_event_info_t info);
    void onScanDone(const wifi_event_sta_scan_done_t &scan_done);
    void startScanning();
};
}
#endif
