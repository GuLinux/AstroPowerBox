#include "commandparser.h"
#include "pwm_output.h"
#include <ArduinoLog.h>
#define LOG_SCOPE "APB::CommandParser "

using namespace APB;

CommandParser &CommandParser::Instance = *new CommandParser();

CommandParser::CommandParser() {
}

JsonResponse CommandParser::getPWMOutputs() {
    Log.infoln(LOG_SCOPE "onGetPWMOutputs: %d", PWMOutputs::Instance.size());
    JsonResponse response;
    PWMOutputs::toJson(response.root()["pwmOutputs"].to<JsonArray>());
    return response;
}

JsonResponse APB::CommandParser::setPWMOutputs(Validation &validation) {
    JsonObject json = validation.json();
    if(validation.required<int>("index").required<const char*>("mode")
        .range("index", {0}, {PWMOutputs::Instance.size()-1})
        .range("max_duty", {0}, {1})
        .choice("mode", PWMOutput::validModes()).invalid()) return validation.errorResponse();

    PWMOutput::Mode mode = PWMOutput::modeFromString(json["mode"]);
    if(mode != PWMOutput::Mode::off) {
        if(validation.range("max_duty", {0}, {1}).required<float>("max_duty").invalid()) return validation.errorResponse();
        if(mode == PWMOutput::Mode::dewpoint) {
            if(validation
                .range("dewpoint_offset", {-30}, {30})
                .required<float>("dewpoint_offset")
                .range("min_duty", 0, 1)
                .range("ramp_offset", 0, 20)
                .invalid()
            ) return validation.errorResponse();
        }
        if(mode == PWMOutput::Mode::target_temperature) {
            if(validation
                .range("target_temperature", {-50}, {50})
                .required<float>("target_temperature")
                .range("min_duty", 0, 1)
                .range("ramp_offset", 0, 20)
                .invalid()
            ) return validation.errorResponse();
        }
    }

    PWMOutput &pwmOutput = PWMOutputs::Instance[json["index"]];
    const char *errorMessage =  pwmOutput.setState(json);
    if(errorMessage) {
        return JsonResponse::error(JsonResponse::InternalError, errorMessage);
    }
    if(pwmOutput.applyAtStartup()) {
        PWMOutputs::saveConfig();
    }
    return getPWMOutputs();
}
