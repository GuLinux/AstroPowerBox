#ifndef APB_JSON_RESPONSE_H
#define APB_JSON_RESPONSE_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define JSON_CONTENT_TYPE "application/json"

namespace APB {   
    struct JsonResponse {
        JsonResponse(AsyncWebServerRequest *request, int statusCode=200, bool isArray = false)
            : request{request}, statusCode{statusCode} {
            response = new AsyncJsonResponse(isArray);
        }

        JsonVariant &root() {
            return response->getRoot();
        }

        ~JsonResponse() {
            response->setCode(statusCode);
            response->setLength();
            request->send(response);
        }

        AsyncWebServerRequest *request;
        AsyncJsonResponse *response;
        int statusCode;

        static JsonResponse error(int statusCode, const String &errorMessage, AsyncWebServerRequest *request) {
            JsonResponse response(request, statusCode);
            response.root()["error"] = errorMessage;
            return response;
        }
    };
}

#endif
