#include "webserver.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ArduinoLog.h>
#include <forward_list>
#include "validation.h"
#include "jsonresponse.h"
#include <esp_system.h>
#include <LittleFS.h>
#include "utils.h"

#define LOG_SCOPE "APB::WebServer "

using namespace std::placeholders;

APB::WebServer::WebServer(Settings &configuration, WiFiManager &wifiManager, Ambient &ambient, Heaters &heaters, PowerMonitor &powerMonitor, Scheduler &scheduler)
    : server(80), configuration(configuration), wifiManager(wifiManager), ambient(ambient), heaters(heaters), powerMonitor(powerMonitor), scheduler(scheduler) {
}


void APB::WebServer::setup() {
    Log.traceln(LOG_SCOPE "Setup");
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([this](){ Log.infoln(LOG_SCOPE "OTA Started"); });
    ElegantOTA.onProgress([this](size_t current, size_t total){
        Log.infoln(LOG_SCOPE "OTA progress: %d%%(%d/%d)", int(current * 100.0 /total), current, total);
    });
    ElegantOTA.onEnd([this](bool success){ Log.infoln(LOG_SCOPE "OTA Finished, success=%d", success); });
    Log.traceln(LOG_SCOPE "ElegantOTA setup");
   
    onJsonRequest("/api/config/accessPoint", std::bind(&APB::WebServer::onConfigAccessPoint, this, _1, _2), HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/station", std::bind(&APB::WebServer::onConfigStation, this, _1, _2), HTTP_POST | HTTP_DELETE);
    server.on("/api/config/write", HTTP_POST, std::bind(&APB::WebServer::onPostWriteConfig, this, _1));
    server.on("/api/config", HTTP_GET, std::bind(&APB::WebServer::onGetConfig, this, _1));
    server.on("/api/info", HTTP_GET, std::bind(&APB::WebServer::onGetESPInfo, this, _1));
    server.on("/api/power", HTTP_GET, std::bind(&APB::WebServer::onGetPower, this, _1));
    server.on("/api/wifi/connect", HTTP_POST, std::bind(&APB::WebServer::onPostReconnectWiFi, this, _1));
    server.on("/api/wifi", HTTP_GET, std::bind(&APB::WebServer::onGetWiFiStatus, this, _1));
    server.on("/api/restart", HTTP_POST, std::bind(&APB::WebServer::onRestart, this, _1));
    
    server.on("/api/status", HTTP_GET, std::bind(&APB::WebServer::onGetStatus, this, _1));
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    onJsonRequest("/api/simulator/ambient", std::bind(&APB::WebServer::onPostAmbientSetSim, this, _1, _2), HTTP_POST);
#endif
#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
    onJsonRequest("/api/simulator/heaters", std::bind(&APB::WebServer::onPostHeaterSetSim, this, _1, _2), HTTP_POST);
#endif
    server.on("/api/ambient", HTTP_GET, std::bind(&APB::WebServer::onGetAmbient, this, _1));
    server.on("/api/heaters", HTTP_GET, std::bind(&APB::WebServer::onGetHeaters, this, _1));
    server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    server.serveStatic("/static", LittleFS, "/web/static").setDefaultFile("index.html");
    onJsonRequest("/api/heater", std::bind(&APB::WebServer::onPostSetHeater, this, _1, _2), HTTP_POST);
 
    Log.infoln(LOG_SCOPE "Setup finished");
    server.begin();
}


void APB::WebServer::onRestart(AsyncWebServerRequest *request) {
    JsonResponse response(request, 100);
    response.document["status"] = "restarting";
    new Task(1000, TASK_ONCE, [](){ esp_restart(); }, &scheduler, true);
}

void APB::WebServer::onGetStatus(AsyncWebServerRequest *request) {
    JsonResponse response(request, 500);
    response.document["status"] = "ok";
    response.document["uptime"] = esp_timer_get_time() / 1000'000.0;
    response.document["has_power_monitor"] = powerMonitor.status().initialised;
    response.document["has_ambient_sensor"] = ambient.isInitialised();
    response.document["has_serial"] = static_cast<bool>(Serial);
}

void APB::WebServer::onGetConfig(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.document["accessPoint"]["essid"] = configuration.apConfiguration().essid;
    response.document["accessPoint"]["psk"] = configuration.apConfiguration().psk;
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        auto station = configuration.station(i);
        response.document["stations"][i]["essid"] = station.essid;
        response.document["stations"][i]["psk"] = station.psk;
    }
}


void APB::WebServer::onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json) {
    if(request->method() == HTTP_DELETE) {
        Log.traceln(LOG_SCOPE "onConfigAccessPoint: method=%d (%s)", request->method(), request->methodToString());
        configuration.setAPConfiguration("", "");
    }
    if(request->method() == HTTP_POST) {
        if(Validation{request, json}.required({"essid", "psk"}).notEmpty("essid").invalid()) return;

        String essid = json["essid"];
        String psk = json["psk"];
        Log.traceln(LOG_SCOPE "onConfigAccessPoint: essid=%s, psk=%s, method=%d (%s)",
            essid.c_str(), psk.c_str(), request->method(), request->methodToString());
        configuration.setAPConfiguration(essid.c_str(), psk.c_str());

    }
    onGetConfig(request);
}



void APB::WebServer::onConfigStation(AsyncWebServerRequest *request, JsonVariant &json) {
    Validation validation{request, json};
    validation.required("index").range("index", {0}, {APB_MAX_STATIONS-1});

    if(request->method() == HTTP_POST) {
        validation.required({"essid", "psk"}).notEmpty("essid");
    }
    if(validation.invalid()) return;
    int stationIndex = json["index"];
    String essid = json["essid"];
    String psk = json["psk"];
    Log.traceln(LOG_SCOPE "onConfigStation: `%d`, essid=`%s`, psk=`%s`, method=%d (%s)", 
        stationIndex, essid.c_str(), psk.c_str(), request->method(), request->methodToString());
    if(request->method() == HTTP_POST) {
        configuration.setStationConfiguration(stationIndex, essid.c_str(), psk.c_str());
    } else if(request->method() == HTTP_DELETE) {
        configuration.setStationConfiguration(stationIndex, "", "");
    }
    onGetConfig(request);
}

void APB::WebServer::onPostWriteConfig(AsyncWebServerRequest *request) {
    configuration.save();
    onGetConfig(request);
}

void APB::WebServer::onGetWiFiStatus(AsyncWebServerRequest *request) {
    JsonResponse response(request, 100);
    response.document["wifi"]["status"] = wifiManager.status()._to_string();
    response.document["wifi"]["essid"] = wifiManager.essid();
    response.document["wifi"]["ip"] = wifiManager.ipAddress();
    response.document["wifi"]["gateway"] = wifiManager.gateway();
}

void APB::WebServer::onPostReconnectWiFi(AsyncWebServerRequest *request) {
    wifiManager.reconnect();
    onGetConfig(request);
}

void APB::WebServer::onGetAmbient(AsyncWebServerRequest *request) {
    if(!ambient.reading()) {
        JsonResponse::error(500, "Ambient reading not available", request);
        return;
    }
    JsonResponse response(request, 100);
    response.document["temperature"] = ambient.reading()->temperature;
    response.document["humidity"] = ambient.reading()->humidity;
    response.document["dewpoint"] = ambient.reading()->dewpoint();
}



void APB::WebServer::onGetHeaters(AsyncWebServerRequest *request) {
    JsonResponse response(request, heaters.size() * 100);
    std::for_each(heaters.begin(), heaters.end(), [&response](Heater &heater) {
        response.document[heater.index()]["mode"] = heater.mode()._to_string();
        response.document[heater.index()]["duty"] = heater.duty();
        response.document[heater.index()]["active"] = heater.active();
        response.document[heater.index()]["has_temperature"] = heater.temperature().has_value();
        optional::if_present(heater.temperature(), [&](float v){ response.document[heater.index()]["temperature"] = v; });
        optional::if_present(heater.targetTemperature(), [&](float v){ response.document[heater.index()]["target_temperature"] = v; });
        optional::if_present(heater.dewpointOffset(), [&](float v){ response.document[heater.index()]["dewpoint_offset"] = v; });
    });
}


void APB::WebServer::onGetPower(AsyncWebServerRequest *request) {
    if(!powerMonitor.status().initialised) {
        JsonResponse::error(500, "Power reading not available", request);
        return;
    }
    JsonResponse response(request, 200);
    response.document["busVoltage"] = powerMonitor.status().busVoltage;
    response.document["current"] = powerMonitor.status().current;
    response.document["power"] = powerMonitor.status().power;
    response.document["shuntVoltage"] = powerMonitor.status().shuntVoltage;
}


void APB::WebServer::onGetESPInfo(AsyncWebServerRequest *request) {
    JsonResponse response(request, 500);
    response.document["mem"]["freeHeap"] = ESP.getFreeHeap();
    response.document["mem"]["freePsRam"] = ESP.getFreePsram();
    response.document["mem"]["heapSize"] = ESP.getHeapSize();
    response.document["mem"]["psRamSize"] = ESP.getPsramSize();
    response.document["mem"]["usedHeap"] = ESP.getHeapSize() - ESP.getFreeHeap();
    response.document["mem"]["usedPsRam"] = ESP.getPsramSize() - ESP.getFreePsram();
    response.document["mem"]["maxAllocHeap"] = ESP.getMaxAllocHeap();
    response.document["mem"]["maxAllocPsRam"] = ESP.getMaxAllocPsram();
    response.document["sketch"]["MD5"] = ESP.getSketchMD5();
    response.document["sketch"]["size"] = ESP.getSketchSize();
    response.document["sketch"]["freeSketchSpace"] = ESP.getFreeSketchSpace();
}

void APB::WebServer::onPostSetHeater(AsyncWebServerRequest *request, JsonVariant &json) {
    Validation validation{request, json};
    std::forward_list<String> valid_modes;
    std::transform(Heater::Mode::_values().begin(), Heater::Mode::_values().end(), std::front_inserter(valid_modes), std::bind(&Heater::Mode::_to_string, _1));
    if(validation.required({"index", "mode"})
        .range("index", {0}, {heaters.size()-1})
        .range("duty", {0}, {1})
        .choice("mode", valid_modes).invalid()) return;
    Heater &heater = heaters[json["index"]];
    Heater::Mode mode = Heater::Mode::_from_string(json["mode"]);
    if(mode == +Heater::Mode::off) {
        heater.setDuty(0);
        onGetHeaters(request);
        return;
    }
    
    if(validation.range("duty", {0}, {1}).required("duty").invalid()) return;
    float duty = json["duty"];
    static const char *temperatureErrorMessage = "Unable to set target temperature. Heater probably doesn't have a temperature sensor.";
    static const char *dewpointTemperatureErrorMessage = "Unable to set target temperature. Either the heater doesn't have a temperature sensor, or you're missing an ambient sensor.";

    if(mode == +Heater::Mode::fixed) {
        heater.setDuty(json["duty"]);
    }
    if(mode == +Heater::Mode::dewpoint) {
        if(validation.range("dewpoint_offset", {-30}, {30}).required("dewpoint_offset").invalid()) return;
        float dewpointOffset = json["dewpoint_offset"];
        if(!heater.setDewpoint(dewpointOffset, duty)) {
            JsonResponse::error(500, dewpointTemperatureErrorMessage, request);
            return;
        }
    }
    if(mode == +Heater::Mode::target_temperature) {
        if(validation.range("target_temperature", {-50}, {50}).required("target_temperature").invalid()) return;
        float targetTemperature = json["target_temperature"];
        if(!heater.setTemperature(targetTemperature, duty)) {
            JsonResponse::error(500, temperatureErrorMessage, request);
            return;
        }
    }
    onGetHeaters(request);
}

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
void APB::WebServer::onPostAmbientSetSim(AsyncWebServerRequest *request, JsonVariant &json) {
    if(Validation{request, json}
        .required({"temperature", "humidity"})
        .invalid() ) return;
    bool initialised = json.containsKey("initialised") ? json["initialised"] : true;
    ambient.setSim(json["temperature"], json["humidity"], initialised);
    onGetAmbient(request);
}
#endif

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
void APB::WebServer::onPostHeaterSetSim(AsyncWebServerRequest *request, JsonVariant &json) {
    if(Validation{request, json}.required("index").range("index", {0}, {heaters.size()-1}).invalid() ) return;
    auto temperature = json.containsKey("temperature") ? std::optional<float>{ json["temperature"]} : std::optional<float>{};
    heaters[json["index"]].setSimulation(temperature);
    onGetHeaters(request);
}
#endif



void APB::WebServer::onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite method) {
    auto handler = new AsyncCallbackJsonWebHandler(path, f);
    handler->setMethod(method);
    server.addHandler(handler);
}
