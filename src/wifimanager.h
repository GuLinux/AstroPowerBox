#ifndef APB_WIFIMANAGER_H
#define APB_WIFIMANAGER_H

#include <WiFiMulti.h>
#include <enum.h>

#include "settings.h"

namespace APB {
BETTER_ENUM(WifiManager_WiFi_Status, uint8_t, Idle, Connecting, Station, AccessPoint, Error)
class WiFiManager {
public:
    using Status = WifiManager_WiFi_Status;
    WiFiManager(Settings &configuration);
    void setup();
    void connect();
    Status status() const { return _status; }
private:
    Settings &configuration;
    WiFiMulti wifiMulti;
    Status _status;

    void onEvent(arduino_event_id_t event, arduino_event_info_t info);
};
}
#endif
