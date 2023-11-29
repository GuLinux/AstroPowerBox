#include "webserver.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

#define LOG_SCOPE F("APB::Configuration")
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

APB::WebServer::WebServer(logging::Logger &logger, Settings &configuration, WiFiManager &wifiManager, Ambient &ambient)
    : server(80), logger(logger), configuration(configuration), wifiManager(wifiManager), ambient(ambient) {
}


void APB::WebServer::setup() {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "Setup");
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([this](){ logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "OTA Started"); });
    ElegantOTA.onProgress([this](size_t current, size_t total){
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "OTA progress: %d%%(%d/%d)", int(current/total), current, total);
    });
    ElegantOTA.onEnd([this](bool success){ logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "OTA Finished, success=%d", success); });
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "ElegantOTA setup");
   
    onJsonRequest("/api/config/accessPoint", std::bind(&APB::WebServer::onConfigAccessPoint, this, _1, _2), HTTP_POST | HTTP_DELETE);
    onJsonRequest("/api/config/station", std::bind(&APB::WebServer::onConfigStation, this, _1, _2), HTTP_POST | HTTP_DELETE);
    server.on("/api/config/write", HTTP_POST, std::bind(&APB::WebServer::onPostWriteConfig, this, _1));
    server.on("/api/config", HTTP_GET, std::bind(&APB::WebServer::onGetConfig, this, _1));
    server.on("/api/wifi/connect", HTTP_POST, std::bind(&APB::WebServer::onPostReconnectWiFi, this, _1));
    server.on("/api/wifi", HTTP_GET, std::bind(&APB::WebServer::onGetWiFiStatus, this, _1));
    
    server.on("/api/status", HTTP_GET, std::bind(&APB::WebServer::onGetStatus, this, _1));
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    onJsonRequest("/api/simulator/ambient", std::bind(&APB::WebServer::onPostAmbientSetSim, this, _1, _2), HTTP_POST);
#endif
    server.on("/api/ambient", HTTP_GET, std::bind(&APB::WebServer::onGetAmbient, this, _1));
 
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Setup finished");
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
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "onConfigAccessPoint: method=%d (%s)", request->method(), request->methodToString());
        configuration.setAPConfiguration("", "");
    }
    if(request->method() == HTTP_POST) {
        if(!checkMandatoryParameter(request, json, "essid")) return;
        if(!checkMandatoryParameter(request, json, "psk")) return;

        String essid = json["essid"];
        String psk = json["psk"];
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "onConfigAccessPoint: essid=%s, psk=%s, method=%d (%s)",
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
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, LOG_SCOPE, "onConfigStation: `%d`, essid=`%s`, psk=`%s`, method=%d (%s)", 
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



#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
void APB::WebServer::onPostAmbientSetSim(AsyncWebServerRequest *request, JsonVariant &json) {
    if(!checkMandatoryParameter(request, json, "temperature")) return;
    if(!checkMandatoryParameter(request, json, "humidity")) return;
    ambient.setSim(json["temperature"], json["humidity"]);
    onGetAmbient(request);
}
#endif


void APB::WebServer::onJsonRequest(const char *path, ArJsonRequestHandlerFunction f, WebRequestMethodComposite method) {
    auto handler = new AsyncCallbackJsonWebHandler(path, f);
    handler->setMethod(method);
    server.addHandler(handler);
}

