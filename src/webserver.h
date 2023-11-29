#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <logger.h>
#include <ArduinoJson.h>

#include "settings.h"
#include "wifimanager.h"

namespace APB {

class WebServer {
public:
    WebServer(logging::Logger &logger, Settings &configuration, WiFiManager &wifiManager);
    void setup();
private:
    AsyncWebServer server;
    logging::Logger &logger;
    Settings &configuration;
    WiFiManager &wifiManager;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
};
}

#endif