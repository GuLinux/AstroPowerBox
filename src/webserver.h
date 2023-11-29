#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <logger.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "settings.h"
#include "wifimanager.h"
#include "ambient.h"

namespace APB {

class WebServer {
public:
    WebServer(logging::Logger &logger, Settings &configuration, WiFiManager &wifiManager, Ambient &ambient);
    void setup();
private:
    AsyncWebServer server;
    logging::Logger &logger;
    Settings &configuration;
    WiFiManager &wifiManager;
    Ambient &ambient;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
    void onGetAmbient(AsyncWebServerRequest *request);

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    void onPostAmbientSetSim(AsyncWebServerRequest *request, JsonVariant &json);
#endif
    void onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite = HTTP_POST | HTTP_PUT | HTTP_PATCH);
};
}

#endif