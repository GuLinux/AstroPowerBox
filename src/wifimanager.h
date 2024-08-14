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
    using Status = WifiManager_WiFi_Status;
    WiFiManager(Settings &configuration, StatusLed &led, Scheduler &scheduler);
    void setup();
    
    void reconnect() { scheduleReconnect = true; }
    Status status() const { return _status; }
    String essid() const;
    String ipAddress() const;
    String gateway() const;
    void loop();
private:
    Settings &configuration;
    StatusLed &led;
    WiFiMulti wifiMulti;
    Status _status;
    bool scheduleReconnect = false;
    bool connectionFailed = false;
    Task rescanWiFiTask;

    void connect();
    void setApMode();
    void onEvent(arduino_event_id_t event, arduino_event_info_t info);
    void onScanDone(const wifi_event_sta_scan_done_t &scan_done);
    void startScanning();
};
}
#endif
