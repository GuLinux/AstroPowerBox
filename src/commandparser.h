#pragma once
#include <ArduinoJson.h>

namespace APB {
class CommandParser {
public:
    CommandParser();
    static CommandParser &Instance;
    void getPWMOutputs(JsonArray response);
    void setPWMOutputs(JsonVariant request);
};
}