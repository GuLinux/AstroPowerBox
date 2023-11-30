#include "webserver.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ArduinoLog.h>

#define LOG_SCOPE "APB::Configuration"
#define JSON_CONTENT_TYPE "application/json"

using namespace std::placeholders;

namespace {   
    struct JsonResponse {
        JsonResponse(AsyncWebServerRequest *request, size_t bufferSize=512, int statusCode=200)
            : document(bufferSize), request{request}, statusCode{statusCode} {
        }

        ~JsonResponse() {
            AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT_TYPE);
            serializeJson(document, *response);
            response->setCode(statusCode);
            request->send(response);
        }

        ArduinoJson::DynamicJsonDocument document;
        AsyncWebServerRequest *request;
        int statusCode;

        static JsonResponse error(int statusCode, const String &errorMessage, AsyncWebServerRequest *request, size_t bufferSize=512) {
            JsonResponse response(request, bufferSize, statusCode);
            response.document["error"] = errorMessage;
            return response;
        }
    };

    bool checkMandatoryParameter(AsyncWebServerRequest *request, JsonVariant &json, const char *key, const char *keyForErrorMessage=nullptr) {
        if(json.containsKey(key)) {
            return true;
        }
        char errorMessage[100];
        sprintf(errorMessage, "Missing mandatory parameter: `%s`", (keyForErrorMessage ? keyForErrorMessage : key));
        JsonResponse::error(400, errorMessage, request);
        return false;
    }
}

APB::WebServer::WebServer(Settings &configuration, WiFiManager &wifiManager, Ambient &ambient, Heaters &heaters)
    : server(80), configuration(configuration), wifiManager(wifiManager), ambient(ambient), heaters(heaters) {
}


void APB::WebServer::setup() {
    Log.traceln(LOG_SCOPE "Setup");
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([this](){ Log.infoln(LOG_SCOPE "OTA Started"); });
    ElegantOTA.onProgress([this](size_t current, size_t total){
        Log.infoln(LOG_SCOPE "OTA progress: %d%%(%d/%d)", int(current/total), current, total);
    });
    ElegantOTA.onEnd([this](bool success){ Log.infoln(LOG_SCOPE "OTA Finished, success=%d", success); });
    Log.traceln(LOG_SCOPE "ElegantOTA setup");
   
    onJsonRequest("/api/config/accessPoint", std::bind(&APB::WebServer::onConfigAccessPoint, this, _1, _2), HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/station", std::bind(&APB::WebServer::onConfigStation, this, _1, _2), HTTP_POST | HTTP_DELETE);
    server.on("/api/config/write", HTTP_POST, std::bind(&APB::WebServer::onPostWriteConfig, this, _1));
    server.on("/api/config", HTTP_GET, std::bind(&APB::WebServer::onGetConfig, this, _1));
    server.on("/api/info", HTTP_GET, std::bind(&APB::WebServer::onGetESPInfo, this, _1));
    server.on("/api/wifi/connect", HTTP_POST, std::bind(&APB::WebServer::onPostReconnectWiFi, this, _1));
    server.on("/api/wifi", HTTP_GET, std::bind(&APB::WebServer::onGetWiFiStatus, this, _1));
    
    server.on("/api/status", HTTP_GET, std::bind(&APB::WebServer::onGetStatus, this, _1));
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    onJsonRequest("/api/simulator/ambient", std::bind(&APB::WebServer::onPostAmbientSetSim, this, _1, _2), HTTP_POST);
#endif
#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
    onJsonRequest("/api/simulator/heaters", std::bind(&APB::WebServer::onPostHeaterSetSim, this, _1, _2), HTTP_POST);
#endif
    server.on("/api/ambient", HTTP_GET, std::bind(&APB::WebServer::onGetAmbient, this, _1));
    server.on("/api/heaters", HTTP_GET, std::bind(&APB::WebServer::onGetHeaters, this, _1));
 
    Log.infoln(LOG_SCOPE "Setup finished");
    server.begin();
}


void APB::WebServer::onGetStatus(AsyncWebServerRequest *request) {
    JsonResponse response(request, 100);
    response.document["status"] = "ok";
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
        if(!checkMandatoryParameter(request, json, "essid")) return;
        if(!checkMandatoryParameter(request, json, "psk")) return;

        String essid = json["essid"];
        String psk = json["psk"];
        Log.traceln(LOG_SCOPE "onConfigAccessPoint: essid=%s, psk=%s, method=%d (%s)",
            essid.c_str(), psk.c_str(), request->method(), request->methodToString());
        if(essid.isEmpty()) {
            JsonResponse::error(400, "ESSID must not be empty", request);
            return;
        }
        configuration.setAPConfiguration(essid.c_str(), psk.c_str());

    }
    onGetConfig(request);
}



void APB::WebServer::onConfigStation(AsyncWebServerRequest *request, JsonVariant &json) {
    if(!checkMandatoryParameter(request, json, "index")) return;
    if(request->method() != HTTP_DELETE) {
        if(!checkMandatoryParameter(request, json, "essid")) return;
        if(!checkMandatoryParameter(request, json, "psk")) return;
    }
    int stationIndex = json["index"];
    String essid = json["essid"];
    String psk = json["psk"];
    Log.traceln(LOG_SCOPE "onConfigStation: `%d`, essid=`%s`, psk=`%s`, method=%d (%s)", 
        stationIndex, essid.c_str(), psk.c_str(), request->method(), request->methodToString());
    if(stationIndex < 0 || stationIndex >= APB_MAX_STATIONS) {
        JsonResponse::error(400, "Invalid station index", request);
        return;
    }
    if(request->method() == HTTP_POST) {
        if(essid.isEmpty()) {
            JsonResponse::error(400, "ESSID must not be empty", request);
            return;
        }

        configuration.setStationConfiguration(stationIndex, essid.c_str(), psk.c_str());
    } 
    if(request->method() == HTTP_DELETE) {
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
}

void APB::WebServer::onPostReconnectWiFi(AsyncWebServerRequest *request) {
    wifiManager.connect();
    onGetConfig(request);
}

void APB::WebServer::onGetAmbient(AsyncWebServerRequest *request) {
    JsonResponse response(request, 100);
    response.document["temperature"] = ambient.temperature();
    response.document["humidity"] = ambient.humidity();
    response.document["dewpoint"] = ambient.dewpoint();
}


void APB::WebServer::onGetHeaters(AsyncWebServerRequest *request) {
    JsonResponse response(request, heaters.size() * 100);
    std::for_each(heaters.begin(), heaters.end(), [&response](Heater &heater) {
        response.document[heater.index()]["mode"] = heater.mode()._to_string();
        response.document[heater.index()]["pwm"] = heater.pwm();
        response.document[heater.index()]["has_temperature"] = heater.temperature().has_value();
        if(heater.temperature().has_value()) {
            response.document[heater.index()]["temperature"] = heater.temperature().value();
        }
        
    });
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



#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
void APB::WebServer::onPostAmbientSetSim(AsyncWebServerRequest *request, JsonVariant &json) {
    if(!checkMandatoryParameter(request, json, "temperature")) return;
    if(!checkMandatoryParameter(request, json, "humidity")) return;
    ambient.setSim(json["temperature"], json["humidity"]);
    onGetAmbient(request);
}
#endif

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
void APB::WebServer::onPostHeaterSetSim(AsyncWebServerRequest *request, JsonVariant &json) {
    if(!checkMandatoryParameter(request, json, "index")) return;
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

