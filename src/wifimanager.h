#ifndef APB_WIFIMANAGER_H
#define APB_WIFIMANAGER_H

#include <WiFiMulti.h>
#include <enum.h>
#include <statusled.h>
#include <vector>
#include "settings.h"
#include <TaskSchedulerDeclarations.h>

namespace APB {
BETTER_ENUM(WifiManager_WiFi_Status, uint8_t, Idle, Connecting, Station, AccessPoint, Error)
class WiFiManager {
public:
    using OnConnectCallback = std::function<void()>;
    static WiFiManager &Instance;
    using Status = WifiManager_WiFi_Status;
    WiFiManager();
    void setup(Scheduler &scheduler);
    
    void reconnect() { scheduleReconnect = true; }
    Status status() const { return _status; }
    String essid() const;
    String ipAddress() const;
    String gateway() const;
    void loop();
    void addOnConnectedListener(const OnConnectCallback &onConnected);
private:
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
