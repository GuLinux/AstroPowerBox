#pragma once
#include <ArduinoJson.h>

namespace APB {
class CommandParser {
public:
    CommandParser();
    static CommandParser &Instance;
    void getHeaters(JsonArray response);
    void setHeaters(JsonVariant request);
};
}