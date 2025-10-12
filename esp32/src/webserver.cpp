#include "webserver.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ArduinoLog.h>
#include <forward_list>
#include <webvalidation.h>
#include <jsonwebresponse.h>
#include "metricsresponse.h"
#include <esp_system.h>
#include <LittleFS.h>
#include <map>
#include "utils.h"
#include "commandparser.h"

#define LOG_SCOPE "APB::WebServer "

using namespace std::placeholders;
using namespace GuLinux;

APB::WebServer::WebServer(Scheduler &scheduler) : AsyncWebServerBase{},
    events("/api/events"),
    scheduler(scheduler) {
}



void APB::WebServer::setup() {
    Log.traceln(LOG_SCOPE "Setup");
    setupCors();
    setupElegantOTA();
    setupJsonNotFoundPage();
#ifdef ALLOW_ALL_CORS
    setupCors();
#endif

    onJsonRequest("/api/config/accessPoint", [](AsyncWebServerRequest *request, JsonVariant &json){ WiFiManager::Instance.onConfigAccessPoint(request, json); }, HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/station", [](AsyncWebServerRequest *request, JsonVariant &json){ WiFiManager::Instance.onConfigStation(request, json); }, HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/statusLedDuty", std::bind(&WebServer::onConfigStatusLedDuty, this, _1, _2), HTTP_POST);
    onJsonRequest("/api/config/fanDuty", std::bind(&WebServer::onConfigFanDuty, this, _1, _2), HTTP_POST);
    onJsonRequest("/api/config/pdVoltage", std::bind(&WebServer::onConfigPDVoltage, this, _1, _2), HTTP_POST);
    onJsonRequest("/api/config/powerSourceType", std::bind(&WebServer::onConfigPowerSourceType, this, _1, _2), HTTP_POST);
    webserver.on("/api/metrics", HTTP_GET, std::bind(&WebServer::onGetMetrics, this, _1));
    webserver.on("/api/config/write", HTTP_POST, std::bind(&WebServer::onPostWriteConfig, this, _1));
    webserver.on("/api/config", HTTP_GET, std::bind(&WebServer::onGetConfig, this, _1));
    webserver.on("/api/info", HTTP_GET, std::bind(&WebServer::onGetESPInfo, this, _1));
    webserver.on("/api/history", HTTP_GET, std::bind(&WebServer::onGetHistory, this, _1));
    webserver.on("/api/power", HTTP_GET, std::bind(&WebServer::onGetPower, this, _1));
    webserver.on("/api/wifi/connect", HTTP_POST, std::bind(&WiFiManager::onPostReconnectWiFi, &WiFiManager::Instance, _1));
    #ifdef CONFIGURATION_FOR_PROTOTYPE
    server.on("/api/wifi", HTTP_DELETE, [this](AsyncWebServerRequest *request){
        new Task(1'000, TASK_ONCE, [](){WiFi.disconnect();}, &scheduler, true);
        JsonWebResponse response(request);
        response.root()["status"] = "Dropping WiFi";
    });
    #endif
    webserver.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request){ WiFiManager::Instance.onGetWiFiStatus(request); });
    webserver.on("/api/restart", HTTP_POST, std::bind(&WebServer::onRestart, this, _1));
    
    webserver.on("/api/status", HTTP_GET, std::bind(&WebServer::onGetStatus, this, _1));
    webserver.on("/api/ambient", HTTP_GET, std::bind(&WebServer::onGetAmbient, this, _1));
    webserver.on("/api/pwmOutputs", HTTP_GET, std::bind(&WebServer::onGetPWMOutputs, this, _1));
    onJsonRequest("/api/pwmOutput", std::bind(&APB::WebServer::onPostSetPWMOutputs, this, _1, _2), HTTP_POST);

    events.onConnect([](AsyncEventSourceClient *client){
        Log.infoln(LOG_SCOPE "[SSE] Client connected: lastId=%d, %s", client->lastId(), client->client()->remoteIP().toString().c_str());
    });
    events.onDisconnect([](AsyncEventSourceClient *client){
        Log.infoln(LOG_SCOPE "[SSE] Client disconnected");
    });
    webserver.addHandler(&events);
    
    webserver.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    webserver.serveStatic("/static", LittleFS, "/web/static").setDefaultFile("index.html");
    
    
 
    Log.infoln(LOG_SCOPE "Setup finished");
    webserver.begin();

    new Task(1000, TASK_FOREVER, [this](){
        eventsDocument.clear();
        if(Ambient::Instance.reading().has_value()) {
            Ambient::Instance.toJson(eventsDocument["ambient"].to<JsonObject>());
        } else {
            eventsDocument["ambient"] = static_cast<char*>(0);
        }
        
        PowerMonitor::Instance.toJson(eventsDocument["power"].to<JsonObject>());
        PWMOutputs::toJson(eventsDocument["pwmOutputs"].to<JsonArray>());
        eventsDocument["app"]["uptime"] = esp_timer_get_time() / 1000'000.0;
        serializeJson(eventsDocument, eventsString.data(), eventsString.size());
        this->events.send(eventsString.data(), "status", millis(), 5000);
    }, &scheduler, true);
}


void APB::WebServer::onRestart(AsyncWebServerRequest *request) {
    JsonWebResponse response(request);
    response.root()["status"] = "restarting";
    new Task(3000, TASK_ONCE, [](){ esp_restart(); }, &scheduler, true);
}

void APB::WebServer::onGetStatus(AsyncWebServerRequest *request) {
    JsonWebResponse response(request);
    response.root()["status"] = "ok";
    response.root()["uptime"] = esp_timer_get_time() / 1000'000.0;

    response.root()["has_power_monitor"] = PowerMonitor::Instance.status().initialised;
    response.root()["has_ambient_sensor"] = Ambient::Instance.isInitialised();
    response.root()["has_serial"] = static_cast<bool>(Serial);
    response.root()["pdVoltageRequested"] = PDProtocol::getVoltage();
}

void APB::WebServer::onGetConfig(AsyncWebServerRequest *request) {
    JsonWebResponse response(request);
    JsonObject rootObject = response.root().to<JsonObject>();
    WiFiManager::Instance.onGetConfig(rootObject);
    rootObject["ledDuty"] = Settings::Instance.statusLedDuty();
    #ifdef APB_PWM_FAN_PIN
    rootObject["fanDuty"] = Settings::Instance.fanDuty();
    #endif
    rootObject["pdVoltage"] = Settings::Instance.pdVoltage();
    rootObject["powerSourceType"] = Settings::PowerSourcesNames.at(Settings::Instance.powerSource());
}

void APB::WebServer::onGetHistory(AsyncWebServerRequest *request) {
    auto jsonSerialiser = std::make_shared<History::JsonSerialiser>(History::Instance);
   
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json",
        [jsonSerialiser](uint8_t *buffer, size_t maxLen, size_t index){
            return jsonSerialiser->write(buffer, maxLen, index);
        });
    request->send(response);
}

void APB::WebServer::onPostWriteConfig(AsyncWebServerRequest *request) {
    Settings::Instance.save();
    onGetConfig(request);
}

void APB::WebServer::onGetAmbient(AsyncWebServerRequest *request) {
    if(!Ambient::Instance.reading()) {
        JsonWebResponse::error(JsonWebResponse::InternalError, "Ambient reading not available", request);
        return;
    }
    JsonWebResponse response(request);
    Ambient::Instance.toJson(response.root().to<JsonObject>());
}

void APB::WebServer::onGetPower(AsyncWebServerRequest *request) {
    if(!PowerMonitor::Instance.status().initialised) {
        JsonWebResponse::error(JsonWebResponse::InternalError, "Power reading not available", request);
        return;
    }
    JsonWebResponse response(request);
    PowerMonitor::Instance.toJson(response.root().to<JsonObject>());
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
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [&metricsResponse](const PWMOutput &pwmOutput) {
        metricsResponse.gauge("pwmOutput", pwmOutput.maxDuty(), MetricsResponse::Labels()
            .add("index", String(pwmOutput.index()).c_str())
            .field("maxDuty")
            .add("mode", pwmOutput.modeAsString().c_str()), nullptr, pwmOutput.index() ==0);
    });
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [&metricsResponse](const PWMOutput &pwmOutput) {
        metricsResponse.gauge("pwmOutput", pwmOutput.duty(), MetricsResponse::Labels()
            .add("index", String(pwmOutput.index()).c_str())
            .field("duty")
            .add("mode", pwmOutput.modeAsString().c_str()), nullptr, false);
    });

    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [&metricsResponse](const PWMOutput &pwmOutput) {
        metricsResponse.gauge("pwmOutput", pwmOutput.active(), MetricsResponse::Labels()
            .add("index", String(pwmOutput.index()).c_str())
            .field("active")
            .add("mode", pwmOutput.modeAsString().c_str()), nullptr, false);
    });
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [&metricsResponse](const PWMOutput &pwmOutput) {
        if(pwmOutput.temperature().has_value()) {
            metricsResponse.gauge("pwmOutput", pwmOutput.temperature().value(), MetricsResponse::Labels()
                .add("index", String(pwmOutput.index()).c_str())
                .unit("°C")
                .field("temperature")
                .add("mode", pwmOutput.modeAsString().c_str()), nullptr, false);
        }
    });
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [&metricsResponse](const PWMOutput &pwmOutput) {
        if(pwmOutput.targetTemperature().has_value()) {
            metricsResponse.gauge("pwmOutput_target_temperature", pwmOutput.targetTemperature().value(), MetricsResponse::Labels()
                .add("index", String(pwmOutput.index()).c_str())
                .field("target_temperature")
                .unit("°C")
                .add("mode", pwmOutput.modeAsString().c_str()), nullptr, false);
        }
    });
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [index=0, &metricsResponse](const PWMOutput &pwmOutput) mutable {
        if(pwmOutput.dewpointOffset().has_value()) {
            metricsResponse.gauge("pwmOutput_dewpoint_offset", pwmOutput.dewpointOffset().value(), MetricsResponse::Labels()
                .add("index", String(pwmOutput.index()).c_str())
                .field("dewpoint_offset")
                .unit("°C")
                .add("mode", pwmOutput.modeAsString().c_str()), nullptr, false);
        }
    });


    metricsResponse.gauge("heap", ESP.getFreeHeap(), MetricsResponse::Labels().field("free"));
    metricsResponse.gauge("heap", ESP.getHeapSize(), MetricsResponse::Labels().field("size"), nullptr, false);
    metricsResponse.gauge("heap", ESP.getMinFreeHeap(), MetricsResponse::Labels().field("min_free"), nullptr, false);
    metricsResponse.gauge("heap", ESP.getMaxAllocHeap(), MetricsResponse::Labels().field("max_alloc"), nullptr, false);
    metricsResponse.gauge("uptime", esp_timer_get_time() / 1000'000.0);
}



void APB::WebServer::onGetESPInfo(AsyncWebServerRequest *request) {
    JsonWebResponse response(request);
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

void APB::WebServer::onPostSetPWMOutputs(AsyncWebServerRequest *request, JsonVariant &json) {
    WebValidation validation{request, json};
    CommandParser::Instance.setPWMOutputs(validation);
    JsonWebResponse response(request, CommandParser::Instance.getPWMOutputs());
}

void APB::WebServer::onGetPWMOutputs(AsyncWebServerRequest *request) {
    auto jsonResponse = CommandParser::Instance.getPWMOutputs();
    JsonWebResponse response(request, jsonResponse);
}



void APB::WebServer::onConfigStatusLedDuty(AsyncWebServerRequest *request, JsonVariant &json) {
    WebValidation validation{request, json};
    if(validation.required<float>("duty")
        .range("duty", {0}, {1})
        .invalid()) return;
    StatusLed::Instance.setDuty(json["duty"]);
    JsonWebResponse response(request);
    response.root()["duty"] = StatusLed::Instance.duty();
}

void APB::WebServer::onConfigPDVoltage(AsyncWebServerRequest *request, JsonVariant &json) {
    WebValidation validation{request, json};
    if(validation.required<PDProtocol::Voltage>("pdVoltage")
        .invalid()) return;
    bool valid_voltage = std::any_of(PDProtocol::getSupportedVoltages().begin(), PDProtocol::getSupportedVoltages().end(),
        [json](PDProtocol::Voltage v){ return v == json["pdVoltage"].as<PDProtocol::Voltage>(); });
    if(!valid_voltage) {
        JsonWebResponse::error(JsonWebResponse::BadRequest, "Invalid voltage", request);
        return;
    }
    Settings::Instance.setPdVoltage(json["pdVoltage"].as<PDProtocol::Voltage>());
    PDProtocol::setVoltage(Settings::Instance.pdVoltage());
    JsonWebResponse response(request);
    response.root()["pdVoltage"] = PDProtocol::getVoltage();
}

void APB::WebServer::onConfigFanDuty(AsyncWebServerRequest *request, JsonVariant &json) {
    #ifndef APB_PWM_FAN_PIN
        JsonWebResponse(request).error(JsonWebResponse::NotFound, "Fan control not supported on this hardware", request);
    #else
        WebValidation validation{request, json};
        if(validation.required<float>("duty")
            .range("duty", {0}, {1})
            .invalid()) return;
        int analogValue = json["duty"].as<float>() * 255.0;
        analogWrite(APB_PWM_FAN_PIN, analogValue);    
        JsonWebResponse response(request);
        response.root()["duty"] = Settings::Instance.fanDuty();
    #endif
}

void APB::WebServer::onConfigPowerSourceType(AsyncWebServerRequest *request, JsonVariant &json) {
    WebValidation validation{request, json};
    std::forward_list<String> choices;
    std::map<String, PowerMonitor::PowerSource> mapping;
    std::for_each(
        Settings::PowerSourcesNames.begin(),
        Settings::PowerSourcesNames.end(),
        [&choices, &mapping](const auto &item){
            choices.push_front(item.second);
            mapping[item.second] = item.first;
        }
    );
    if(validation.required<String>("powerSourceType")
        .choice("powerSourceType", choices)
        .invalid()) return;
    Settings::Instance.setPowerSource(mapping.at(json["powerSourceType"]));
    JsonWebResponse response(request);
    response.root()["powerSourceType"] = Settings::PowerSourcesNames.at(Settings::Instance.powerSource());
}


