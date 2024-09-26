#include "webserver.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ArduinoLog.h>
#include <forward_list>
#include <validation.h>
#include <jsonresponse.h>
#include "metricsresponse.h"
#include <esp_system.h>
#include <LittleFS.h>
#include "utils.h"

#define LOG_SCOPE "APB::WebServer "

using namespace std::placeholders;

APB::WebServer::WebServer(Scheduler &scheduler) : server(80),
    events("/api/events"),
    scheduler(scheduler)
{
}


void APB::WebServer::setup() {
    Log.traceln(LOG_SCOPE "Setup");
    ElegantOTA.begin(&server);
    #ifdef ALLOW_ALL_CORS
    #warning "Adding Access-Control-Allow-Origin:* header to all requests (CORS)"
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    #endif
    ElegantOTA.onStart([this](){ Log.infoln(LOG_SCOPE "OTA Started"); });
    ElegantOTA.onProgress([this](size_t current, size_t total){
        Log.infoln(LOG_SCOPE "OTA progress: %d%%(%d/%d)", int(current * 100.0 /total), current, total);
    });
    ElegantOTA.onEnd([this](bool success){ Log.infoln(LOG_SCOPE "OTA Finished, success=%d", success); });
    Log.traceln(LOG_SCOPE "ElegantOTA setup");
   
    onJsonRequest("/api/config/accessPoint", std::bind(&APB::WebServer::onConfigAccessPoint, this, _1, _2), HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/station", std::bind(&APB::WebServer::onConfigStation, this, _1, _2), HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/statusLedDuty", std::bind(&APB::WebServer::onConfigStatusLedDuty, this, _1, _2), HTTP_POST);
    server.on("/api/metrics", HTTP_GET, std::bind(&APB::WebServer::onGetMetrics, this, _1));
    server.on("/api/config/write", HTTP_POST, std::bind(&APB::WebServer::onPostWriteConfig, this, _1));
    server.on("/api/config", HTTP_GET, std::bind(&APB::WebServer::onGetConfig, this, _1));
    server.on("/api/info", HTTP_GET, std::bind(&APB::WebServer::onGetESPInfo, this, _1));
    server.on("/api/history", HTTP_GET, std::bind(&APB::WebServer::onGetHistory, this, _1));
    server.on("/api/power", HTTP_GET, std::bind(&APB::WebServer::onGetPower, this, _1));
    server.on("/api/wifi/connect", HTTP_POST, std::bind(&APB::WebServer::onPostReconnectWiFi, this, _1));
    #ifdef CONFIGURATION_FOR_PROTOTYPE
    server.on("/api/wifi", HTTP_DELETE, [this](AsyncWebServerRequest *request){
        new Task(1'000, TASK_ONCE, [](){WiFi.disconnect();}, &scheduler, true);
        JsonResponse response(request);
        response.root()["status"] = "Dropping WiFi";
    });
    #endif
    server.on("/api/wifi", HTTP_GET, std::bind(&APB::WebServer::onGetWiFiStatus, this, _1));
    server.on("/api/restart", HTTP_POST, std::bind(&APB::WebServer::onRestart, this, _1));
    
    server.on("/api/status", HTTP_GET, std::bind(&APB::WebServer::onGetStatus, this, _1));
    server.on("/api/ambient", HTTP_GET, std::bind(&APB::WebServer::onGetAmbient, this, _1));
    server.on("/api/heaters", HTTP_GET, std::bind(&APB::WebServer::onGetHeaters, this, _1));
    server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    server.serveStatic("/static", LittleFS, "/web/static").setDefaultFile("index.html");
    server.addHandler(&events);
    server.onNotFound(std::bind(&APB::WebServer::onNotFound, this, _1));
    onJsonRequest("/api/heater", std::bind(&APB::WebServer::onPostSetHeater, this, _1, _2), HTTP_POST);
 
    Log.infoln(LOG_SCOPE "Setup finished");
    server.begin();

    new Task(1000, TASK_FOREVER, [this](){
        eventsDocument.clear();
        if(Ambient::Instance.reading().has_value()) {
            populateAmbientStatus(eventsDocument["ambient"].to<JsonObject>());
        } else {
            eventsDocument["ambient"] = static_cast<char*>(0);
        }
        
        populatePowerStatus(eventsDocument["power"].to<JsonObject>());
        populateHeatersStatus(eventsDocument["heaters"].to<JsonArray>());
        eventsDocument["app"]["uptime"] = esp_timer_get_time() / 1000'000.0;
        serializeJson(eventsDocument, eventsString.data(), eventsString.size());
        this->events.send(eventsString.data(), "status", millis(), 5000);
    }, &scheduler, true);
}


void APB::WebServer::onRestart(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.root()["status"] = "restarting";
    new Task(1000, TASK_ONCE, [](){ esp_restart(); }, &scheduler, true);
}

void APB::WebServer::onGetStatus(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.root()["status"] = "ok";
    response.root()["uptime"] = esp_timer_get_time() / 1000'000.0;

    response.root()["has_power_monitor"] = PowerMonitor::Instance.status().initialised;
    response.root()["has_ambient_sensor"] = Ambient::Instance.isInitialised();
    response.root()["has_serial"] = static_cast<bool>(Serial);
}

void APB::WebServer::onGetConfig(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.root()["accessPoint"]["essid"] = Settings::Instance.wifi().apConfiguration().essid;
    response.root()["accessPoint"]["psk"] = Settings::Instance.wifi().apConfiguration().psk;
    for(uint8_t i=0; i<APB_MAX_STATIONS; i++) {
        auto station = Settings::Instance.wifi().station(i);
        response.root()["stations"][i]["essid"] = station.essid;
        response.root()["stations"][i]["psk"] = station.psk;
    }
    response.root()["ledDuty"] = Settings::Instance.statusLedDuty();
}

void APB::WebServer::onGetHistory(AsyncWebServerRequest *request) {
    auto jsonSerialiser = std::make_shared<History::JsonSerialiser>(History::Instance);
   
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json",
        [jsonSerialiser](uint8_t *buffer, size_t maxLen, size_t index){
            return jsonSerialiser->write(buffer, maxLen, index);
        });
    request->send(response);
}

void APB::WebServer::onNotFound(AsyncWebServerRequest *request) {
    JsonResponse response(request, 404);
    response.root()["error"] = "NotFound";
    response.root()["url"] = request->url();
}

void APB::WebServer::onConfigAccessPoint(AsyncWebServerRequest *request, JsonVariant &json) {
    if(request->method() == HTTP_DELETE) {
        Log.traceln(LOG_SCOPE "onConfigAccessPoint: method=%d (%s)", request->method(), request->methodToString());
        Settings::Instance.wifi().setAPConfiguration("", "");
    }
    if(request->method() == HTTP_POST) {
        if(Validation{request, json}.required<const char*>({"essid", "psk"}).notEmpty("essid").invalid()) return;

        String essid = json["essid"];
        String psk = json["psk"];
        Log.traceln(LOG_SCOPE "onConfigAccessPoint: essid=%s, psk=%s, method=%d (%s)",
            essid.c_str(), psk.c_str(), request->method(), request->methodToString());
        Settings::Instance.wifi().setAPConfiguration(essid.c_str(), psk.c_str());

    }
    onGetConfig(request);
}



void APB::WebServer::onConfigStation(AsyncWebServerRequest *request, JsonVariant &json) {
    Validation validation{request, json};
    validation.required<int>("index").range("index", {0}, {APB_MAX_STATIONS-1});

    if(request->method() == HTTP_POST) {
        validation.required<const char*>({"essid", "psk"}).notEmpty("essid");
    }
    if(validation.invalid()) return;
    int stationIndex = json["index"];
    String essid = json["essid"];
    String psk = json["psk"];
    Log.traceln(LOG_SCOPE "onConfigStation: `%d`, essid=`%s`, psk=`%s`, method=%d (%s)", 
        stationIndex, essid.c_str(), psk.c_str(), request->method(), request->methodToString());
    if(request->method() == HTTP_POST) {
        Settings::Instance.wifi().setStationConfiguration(stationIndex, essid.c_str(), psk.c_str());
    } else if(request->method() == HTTP_DELETE) {
        Settings::Instance.wifi().setStationConfiguration(stationIndex, "", "");
    }
    onGetConfig(request);
}

void APB::WebServer::onPostWriteConfig(AsyncWebServerRequest *request) {
    Settings::Instance.save();
    onGetConfig(request);
}

void APB::WebServer::onGetWiFiStatus(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.root()["wifi"]["status"] = WiFiManager::Instance.statusAsString();
    response.root()["wifi"]["essid"] = WiFiManager::Instance.essid();
    response.root()["wifi"]["ip"] = WiFiManager::Instance.ipAddress();
    response.root()["wifi"]["gateway"] = WiFiManager::Instance.gateway();
}

void APB::WebServer::onPostReconnectWiFi(AsyncWebServerRequest *request) {
    WiFiManager::Instance.reconnect();
    onGetConfig(request);
}

void APB::WebServer::onGetAmbient(AsyncWebServerRequest *request) {
    if(!Ambient::Instance.reading()) {
        JsonResponse::error(500, "Ambient reading not available", request);
        return;
    }
    JsonResponse response(request);
    populateAmbientStatus(response.root().to<JsonObject>());
}



void APB::WebServer::onGetHeaters(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    populateHeatersStatus(response.root().to<JsonArray>());
}

void APB::WebServer::populateHeatersStatus(JsonArray heatersStatus) {
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [heatersStatus](Heater &heater) {
        heatersStatus[heater.index()]["mode"] = heater.modeAsString(),
        heatersStatus[heater.index()]["duty"] = heater.duty();
        heatersStatus[heater.index()]["active"] = heater.active();
        heatersStatus[heater.index()]["has_temperature"] = heater.temperature().has_value();
        optional::if_present(heater.temperature(), [&](float v){ heatersStatus[heater.index()]["temperature"] = v; });
        optional::if_present(heater.targetTemperature(), [&](float v){ heatersStatus[heater.index()]["target_temperature"] = v; });
        optional::if_present(heater.dewpointOffset(), [&](float v){ heatersStatus[heater.index()]["dewpoint_offset"] = v; });
    });
}

void APB::WebServer::populateAmbientStatus(JsonObject ambientStatus) {
    ambientStatus["temperature"] = Ambient::Instance.reading()->temperature;
    ambientStatus["humidity"] = Ambient::Instance.reading()->humidity;
    ambientStatus["dewpoint"] = Ambient::Instance.reading()->dewpoint();
}


void APB::WebServer::populatePowerStatus(JsonObject powerStatus) {
    powerStatus["busVoltage"] = PowerMonitor::Instance.status().busVoltage;
    powerStatus["current"] = PowerMonitor::Instance.status().current;
    powerStatus["power"] = PowerMonitor::Instance.status().power;
    powerStatus["shuntVoltage"] = PowerMonitor::Instance.status().shuntVoltage;
}



void APB::WebServer::onGetPower(AsyncWebServerRequest *request) {
    if(!PowerMonitor::Instance.status().initialised) {
        JsonResponse::error(500, "Power reading not available", request);
        return;
    }
    JsonResponse response(request);
    populatePowerStatus(response.root().to<JsonObject>());
}

void APB::WebServer::onGetMetrics(AsyncWebServerRequest *request) {
    MetricsResponse metricsResponse(request, MetricsResponse::Labels().add("source", Settings::Instance.wifi().hostname()));
    const auto powerMonitorReading = PowerMonitor::Instance.status();
    metricsResponse
        .gauge("powermonitor", powerMonitorReading.busVoltage, MetricsResponse::Labels().unit("V").field("voltage"))
        .gauge("powermonitor", powerMonitorReading.current, MetricsResponse::Labels().unit("A").field("current"), nullptr, false)
        .gauge("powermonitor", powerMonitorReading.power, MetricsResponse::Labels().unit("W").field("power"), nullptr, false);
    
    const auto ambientReading = Ambient::Instance.reading();
    if(ambientReading.has_value()) {
        // Log.traceln("adding ambient metrics data: T=%d, H=%d, D=%d", ambientReading->temperature, ambientReading->humidity, ambientReading->dewpoint());
        metricsResponse
            .gauge("ambient", ambientReading->temperature, MetricsResponse::Labels().unit("°C").field("temperature"))
            .gauge("ambient", ambientReading->humidity, MetricsResponse::Labels().unit("%").field("humidity"), nullptr, false)
            .gauge("ambient", ambientReading->dewpoint(), MetricsResponse::Labels().unit("°C").field("dewpoint"), nullptr, false);
    }
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [index=0, &metricsResponse](const Heater &heater) mutable {
        metricsResponse.gauge("heater", heater.duty(), MetricsResponse::Labels()
            .add("index", String(heater.index()).c_str())
            .field("duty")
            .add("mode", heater.modeAsString().c_str()), nullptr, index++==0);
    });
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [index=0, &metricsResponse](const Heater &heater) mutable {
        metricsResponse.gauge("heater", heater.active(), MetricsResponse::Labels()
            .add("index", String(heater.index()).c_str())
            .field("active")
            .add("mode", heater.modeAsString().c_str()), nullptr, false);
    });
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [index=0, &metricsResponse](const Heater &heater) mutable {
        if(heater.temperature().has_value()) {
            metricsResponse.gauge("heater", heater.temperature().value(), MetricsResponse::Labels()
                .add("index", String(heater.index()).c_str())
                .unit("°C")
                .field("temperature")
                .add("mode", heater.modeAsString().c_str()), nullptr, false);
        }
    });
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [index=0, &metricsResponse](const Heater &heater) mutable {
        if(heater.targetTemperature().has_value()) {
            metricsResponse.gauge("heater_target_temperature", heater.targetTemperature().value(), MetricsResponse::Labels()
                .add("index", String(heater.index()).c_str())
                .field("target_temperature")
                .unit("°C")
                .add("mode", heater.modeAsString().c_str()), nullptr, false);
        }
    });
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [index=0, &metricsResponse](const Heater &heater) mutable {
        if(heater.dewpointOffset().has_value()) {
            metricsResponse.gauge("heater_dewpoint_offset", heater.dewpointOffset().value(), MetricsResponse::Labels()
                .add("index", String(heater.index()).c_str())
                .field("dewpoint_offset")
                .unit("°C")
                .add("mode", heater.modeAsString().c_str()), nullptr, false);
        }
    });


    metricsResponse.gauge("heap", ESP.getFreeHeap(), MetricsResponse::Labels().field("free"));
    metricsResponse.gauge("heap", ESP.getHeapSize(), MetricsResponse::Labels().field("size"), nullptr, false);
    metricsResponse.gauge("heap", ESP.getMinFreeHeap(), MetricsResponse::Labels().field("min_free"), nullptr, false);
    metricsResponse.gauge("heap", ESP.getMaxAllocHeap(), MetricsResponse::Labels().field("max_alloc"), nullptr, false);
    metricsResponse.gauge("uptime", esp_timer_get_time() / 1000'000.0);
}



void APB::WebServer::onGetESPInfo(AsyncWebServerRequest *request) {
    JsonResponse response(request);
    response.root()["mem"]["freeHeap"] = ESP.getFreeHeap();
    response.root()["mem"]["freePsRam"] = ESP.getFreePsram();
    response.root()["mem"]["heapSize"] = ESP.getHeapSize();
    response.root()["mem"]["psRamSize"] = ESP.getPsramSize();
    response.root()["mem"]["usedHeap"] = ESP.getHeapSize() - ESP.getFreeHeap();
    response.root()["mem"]["usedPsRam"] = ESP.getPsramSize() - ESP.getFreePsram();
    response.root()["mem"]["maxAllocHeap"] = ESP.getMaxAllocHeap();
    response.root()["mem"]["maxAllocPsRam"] = ESP.getMaxAllocPsram();
    response.root()["sketch"]["MD5"] = ESP.getSketchMD5();
    response.root()["sketch"]["size"] = ESP.getSketchSize();
    response.root()["sketch"]["totalSpace"] = ESP.getFreeSketchSpace();
    response.root()["sketch"]["sdkVersion"] = ESP.getSdkVersion();
    response.root()["esp"]["chipModel"] = ESP.getChipModel();
    response.root()["esp"]["chipCores"] = ESP.getChipCores();
    response.root()["esp"]["cpuFreqMHz"] = ESP.getCpuFreqMHz();
}

void APB::WebServer::onPostSetHeater(AsyncWebServerRequest *request, JsonVariant &json) {
    Validation validation{request, json};
    if(validation.required<int>("index").required<const char*>("mode")
        .range("index", {0}, {Heaters::Instance.size()-1})
        .range("duty", {0}, {1})
        .choice("mode", Heater::validModes()).invalid()) return;
    Heater &heater = Heaters::Instance[json["index"]];
    Heater::Mode mode = Heater::modeFromString(json["mode"]);
    if(mode == Heater::Mode::off) {
        heater.setDuty(0);
        onGetHeaters(request);
        return;
    }
    
    if(validation.range("duty", {0}, {1}).required<float>("duty").invalid()) return;
    float duty = json["duty"];
    static const char *temperatureErrorMessage = "Unable to set target temperature. Heater probably doesn't have a temperature sensor.";
    static const char *dewpointTemperatureErrorMessage = "Unable to set target temperature. Either the heater doesn't have a temperature sensor, or you're missing an ambient sensor.";

    if(mode == Heater::Mode::fixed) {
        heater.setDuty(json["duty"]);
    }
    if(mode == Heater::Mode::dewpoint) {
        if(validation.range("dewpoint_offset", {-30}, {30}).required<float>("dewpoint_offset").invalid()) return;
        float dewpointOffset = json["dewpoint_offset"];
        if(!heater.setDewpoint(dewpointOffset, duty)) {
            JsonResponse::error(500, dewpointTemperatureErrorMessage, request);
            return;
        }
    }
    if(mode == Heater::Mode::target_temperature) {
        if(validation.range("target_temperature", {-50}, {50}).required<float>("target_temperature").invalid()) return;
        float targetTemperature = json["target_temperature"];
        if(!heater.setTemperature(targetTemperature, duty)) {
            JsonResponse::error(500, temperatureErrorMessage, request);
            return;
        }
    }
    onGetHeaters(request);
}

void APB::WebServer::onConfigStatusLedDuty(AsyncWebServerRequest *request, JsonVariant &json) {
    Validation validation{request, json};
    if(validation.required<float>("duty")
        .range("duty", {0}, {1})
        .invalid()) return;
    StatusLed::Instance.setDuty(json["duty"]);
    JsonResponse response(request);
    response.root()["duty"] = StatusLed::Instance.duty();
}

void APB::WebServer::onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite method) {
    auto handler = new AsyncCallbackJsonWebHandler(path, f);
    handler->setMethod(method);
    server.addHandler(handler);
}
