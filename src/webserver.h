#ifndef APB_WEBSERVER_H
#define APB_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "settings.h"
#include <wifimanager.h>
#include "ambient/ambient.h"
#include "pwm_output.h"
#include "powermonitor.h"
#include <TaskSchedulerDeclarations.h>
#include "statusled.h"
#include "history.h"

#include <AsyncWebServerBase.h>

namespace APB {

class WebServer : public AsyncWebServerBase {
public:
    WebServer(Scheduler &scheduler);
    void setup();
private:
    AsyncEventSource events;
    Scheduler &scheduler;
    JsonDocument eventsDocument;
    std::array<char, MAX_EVENTS_SIZE> eventsString;

    void onGetStatus(AsyncWebServerRequest *request);
    void onGetConfig(AsyncWebServerRequest *request);
    void onGetHistory(AsyncWebServerRequest *request);
    void onNotFound(AsyncWebServerRequest *request);
    void onPostWriteConfig(AsyncWebServerRequest *request);
    void onGetAmbient(AsyncWebServerRequest *request);
    void onGetPower(AsyncWebServerRequest *request);
    void onGetPWMOutputs(AsyncWebServerRequest *request);
    void onGetESPInfo(AsyncWebServerRequest *request);
    void onGetMetrics(AsyncWebServerRequest *request);
    void onRestart(AsyncWebServerRequest *request);
    void onPostSetPWMOutputs(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigStatusLedDuty(AsyncWebServerRequest *request, JsonVariant &json);
    void onConfigPowerSourceType(AsyncWebServerRequest *request, JsonVariant &json);
};
}

#endif