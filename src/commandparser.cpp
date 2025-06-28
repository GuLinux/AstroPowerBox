#include "commandparser.h"
#include "pwm_output.h"
#include <ArduinoLog.h>
#define LOG_SCOPE "APB::CommandParser "

using namespace APB;

CommandParser &CommandParser::Instance = *new CommandParser();

CommandParser::CommandParser() {
}

void CommandParser::getPWMOutputs(JsonArray response) {
    Log.infoln(LOG_SCOPE "onGetPWMOutputs: %d", PWMOutputs::Instance.size());
    PWMOutputs::toJson(response);
}
