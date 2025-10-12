#pragma once
#include <ArduinoJson.h>
#include <validation.h>

namespace APB {
class CommandParser {
public:
    CommandParser();
    static CommandParser &Instance;
    JsonResponse getPWMOutputs();
    JsonResponse setPWMOutputs(Validation &validation);
};
}