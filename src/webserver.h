#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "settings.h"
#include "wifimanager.h"
#include "ambient/ambient.h"
#include "heater.h"
#include "powermonitor.h"
#include <TaskSchedulerDeclarations.h>
#include "statusled.h"
#include "history.h"

namespace APB {

class WebServer {
public:
    WebServer(
        Settings &configuration,
        WiFiManager &wifiManager,
        Ambient &ambient,
        Heaters &heaters,
        PowerMonitor &powerMonitor,
        Scheduler &scheduler,
        StatusLed &statusLed);
    void setup();
private:
    AsyncWebServer server;
    AsyncEventSource events;
    Settings &configuration;
    WiFiManager &wifiManager;
    Ambient &ambient;
    Heaters &heaters;
    PowerMonitor &powerMonitor;
    Scheduler &scheduler;
    StatusLed &statusLed;
    StaticJsonDocument<800> eventsDocument;
    std::array<char, 800> eventsString;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onGetHistory(AsyncWebServerRequest *request);
    void onNotFound(AsyncWebServerRequest *request);
    void onConfigStation(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onPostReconnectWiFi(AsyncWebServerRequest *request);
    void onGetWiFiStatus(AsyncWebServerRequest *request);
    void onGetAmbient(AsyncWebServerRequest *request);
    void onGetPower(AsyncWebServerRequest *request);
    void onGetHeaters(AsyncWebServerRequest *request);
    void onGetESPInfo(AsyncWebServerRequest *request);
    void onRestart(AsyncWebServerRequest *request);
    void onPostSetHeater(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigStatusLedDuty(AsyncWebServerRequest *request, JsonVariant &json);
    

    void populateHeatersStatus(JsonArray heatersStatus);
    void populateAmbientStatus(JsonObject ambientStatus);
    void populatePowerStatus(JsonObject powerStatus);

    void onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite = HTTP_POST | HTTP_PUT | HTTP_PATCH);
};
}

#endif