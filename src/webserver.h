#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "settings.h"
#include "wifimanager.h"
#include "ambient.h"
#include "heater.h"

namespace APB {

class WebServer {
public:
    WebServer(Settings &configuration, WiFiManager &wifiManager, Ambient &ambient, Heaters &heaters);
    void setup();
private:
    AsyncWebServer server;
    Settings &configuration;
    WiFiManager &wifiManager;
    Ambient &ambient;
    Heaters &heaters;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
    void onGetAmbient(AsyncWebServerRequest *request);
    void onGetHeaters(AsyncWebServerRequest *request);

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    void onPostAmbientSetSim(AsyncWebServerRequest *request, JsonVariant &json);
#endif
#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
    void onPostHeaterSetSim(AsyncWebServerRequest *request, JsonVariant &json);
#endif
    void onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite = HTTP_POST | HTTP_PUT | HTTP_PATCH);
};
}

#endif