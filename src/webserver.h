#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "settings.h"
#include "wifimanager.h"
#include "ambient.h"
#include "heater.h"
#include <TaskSchedulerDeclarations.h>

namespace APB {

class WebServer {
public:
    WebServer(Settings &configuration, WiFiManager &wifiManager, Ambient &ambient, Heaters &heaters, Scheduler &scheduler);
    void setup();
private:
    AsyncWebServer server;
    Settings &configuration;
    WiFiManager &wifiManager;
    Ambient &ambient;
    Heaters &heaters;
    Scheduler &scheduler;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
    void onGetAmbient(AsyncWebServerRequest *request);
    void onGetHeaters(AsyncWebServerRequest *request);
    void onGetESPInfo(AsyncWebServerRequest *request);
    void onRestart(AsyncWebServerRequest *request);
    void onPostSetHeater(AsyncWebServerRequest *request, JsonVariant &json);

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