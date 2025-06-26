#include "commandparser.h"
#include "pwm_output.h"
#include <ArduinoLog.h>
#define LOG_SCOPE "APB::CommandParser "

using namespace APB;

CommandParser &CommandParser::Instance = *new CommandParser();

CommandParser::CommandParser() {
}

void CommandParser::getHeaters(JsonArray response) {
    Log.infoln(LOG_SCOPE "onGetHeaters: %d", Heaters::Instance.size());
    Heaters::toJson(response);
}
