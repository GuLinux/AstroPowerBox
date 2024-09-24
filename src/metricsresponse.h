#pragma once

#include <ESPAsyncWebServer.h>
#include <list>
#include <ArduinoLog.h>
#include <StreamString.h>
#include "utils.h"
#include <algorithm>
#include <numeric>

#define METRICS_CONTENT_TYPE "text/plain; version=0.0.4"

namespace APB {   

class MetricsResponse {
public:
    struct Labels {
        StreamString buffer;
        Labels concat(const Labels &other) const {
            Labels newLabels;
            newLabels.add(*this);
            newLabels.add(other);
            return newLabels;
        }


        Labels &add(const Labels &other) {
            if(!other.buffer.isEmpty()) {
                addSeparator();
            }
            buffer.print(other.c_str());
            return *this;
        }

        Labels &add(const char *label, const char *value) {
            addSeparator();
            buffer.printf(R"(%s="%s")", label, value);
            return *this;
        }
        Labels &field(const char *value) {
            return add("field", value);
        }
        Labels &unit(const char *value) {
            return add("unit", value);
        }

        const char *c_str() const {
            return buffer.c_str();
        }

        void addSeparator() {
            if(!buffer.isEmpty()) {
                buffer.write(',');
            }
        }
    };
    MetricsResponse(AsyncWebServerRequest *request, const Labels &fixedLabels, size_t bufferSize=1048 * 10, int statusCode=200)
        : request{request}, response{request->beginResponseStream(METRICS_CONTENT_TYPE)}, fixedLabels{fixedLabels} {
    }

    MetricsResponse &counter(const char *name, float value, const Labels &labels = {}, const char *help = nullptr, bool addHeaders=true) {
        if(addHeaders) {
            addHelp(name, help);
            addType(name, "counter");
        }
        response->printf("%s {%s} %f\n", name, fixedLabels.concat(labels).c_str(), value);
        return *this;
    }

    MetricsResponse &gauge(const char *name, float value, const Labels &labels = {}, const char *help = nullptr, bool addHeaders=true) {
        if(addHeaders) {
            addHelp(name, help);
            addType(name, "gauge");
        }
        
        
        response->printf("%s {%s} %f\n", name, fixedLabels.concat(labels).c_str(), value);
        return *this;
    }

    ~MetricsResponse() {
        request->send(response);
    }

private:
    void addType(const char *name, const char *type) {
        response->printf("# TYPE %s %s\n", name, type);
    }
    void addHelp(const char *name, const char *help) {
        if(help) {
            response->printf("# HELP %s %s\n", name, help);
        }
    }

    AsyncResponseStream *response;
    AsyncWebServerRequest *request;
    const Labels fixedLabels;
};

}
