#ifndef APB_JSON_RESPONSE_H
#define APB_JSON_RESPONSE_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define JSON_CONTENT_TYPE "application/json"

namespace APB {   
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
}

#endif
