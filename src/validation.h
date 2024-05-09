#ifndef APB_VALIDATION_H
#define APB_VALIDATION_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <forward_list>
#include <ArduinoLog.h>
#include "jsonresponse.h"

#define VALIDATION_LOG_SCOPE "APB::Validation"

namespace APB {

struct Validation {
    AsyncWebServerRequest *request;
    JsonVariant &json;
    char errorMessage[512] = {0};
    int status = 400;
    bool valid() const { return strlen(errorMessage) == 0; }
    bool invalid() const { return !valid(); }
    void clear() { errorMessage[0] = 0; }

    Validation &required(const char *key) {
        if(valid() && !json.containsKey(key)) {
            sprintf(errorMessage, "Missing required parameter: `%s`", key);
        }
        return *this;
    }

    Validation &required(const std::forward_list<String> &keys) {
        std::for_each(keys.begin(), keys.end(), [this](const String &key) { required(key.c_str()); });
        return *this;
    }

    Validation &number(const char *key) {
        if(valid() && json.containsKey(key)) {
            if(!json[key].is<float>()) sprintf(errorMessage, "Value for `%s` is not a number", key);
        }
        return *this;
    }

    Validation &range(const char *key, const std::optional<float> &min, const std::optional<float> &max) {
        if(max) Log.traceln(VALIDATION_LOG_SCOPE "min=%F", *min); else Log.traceln(VALIDATION_LOG_SCOPE "min missing");
        if(max) Log.traceln(VALIDATION_LOG_SCOPE "max=%F", *max); else Log.traceln(VALIDATION_LOG_SCOPE "max missing");
        number(key);
        if(valid() && json.containsKey(key)) {
            if(max && json[key] > *max) sprintf(errorMessage, "Value for `%s` greater than allowed max `%f`", key, *max);
            if(min && json[key] < *min) sprintf(errorMessage, "Value for `%s` lower than allowed max `%f`", key, *min);
        }
        return *this;
    }
    Validation &choice(const char *key, const std::forward_list<String> &choices) {
        if(valid() &&
            json.containsKey(key) &&
            std::none_of(choices.begin(), choices.end(), [&key, this](const String &choice){ return choice == json[key]; })) {
            String choicesMessage;
            std::for_each(choices.begin(), choices.end(), [&choicesMessage](const String &choice) {
                if(!choicesMessage.isEmpty()) choicesMessage.concat(", ");
                choicesMessage.concat(choice);
            });
            sprintf(errorMessage, "Invalid value for `%s`. Valid choices: <%s>", key, choicesMessage.c_str());
        }
        return *this;
    }
    Validation &notEmpty(const char *key) {
        if(valid() && json.containsKey(key) && json[key].as<String>().isEmpty()) {
            sprintf(errorMessage, "Parameter `%s` must not be empty", key);
        }
        return *this;
    }

    ~Validation() {
        if(!valid()) {
            JsonResponse::error(status, errorMessage, request);
        }
    }
};
}

#endif
