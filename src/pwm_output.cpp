#include <ArduinoLog.h>
#include <LittleFS.h>
#include <array>

#include "pwm_output.h"
#include "configuration.h"

#ifdef APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>
#endif

#include "settings.h"
#include "utils.h"
#include <unordered_map>

#define PWM_OUT_CONF_FILENAME APB_CONFIG_DIRECTORY "/pwmOutputs.json"


static const char *TEMPERATURE_NOT_FOUND_WARNING_LOG = "%s Cannot set PWM output temperature without temperature sensor";
static const char *AMBIENT_NOT_FOUND_WARNING_LOG = "%s Cannot set PWM output temperature without ambient sensor";

APB::PWMOutputs::Array &APB::PWMOutputs::Instance = *new APB::PWMOutputs::Array();

struct APB::PWMOutput::Private {
    Private(PWMOutput *q, Type type) : q{q}, type{type} {}
    APB::PWMOutput *q;
    Type type;
    
    PWMOutput::Mode mode{PWMOutput::Mode::off};
    float maxDuty;
    float minDuty = 0;
    std::optional<float> temperature;
    float targetTemperature;
    float dewpointOffset;
    float rampOffset = 0;

    bool applyAtStartup = false;

    Task loopTask;
    char log_scope[20];
    uint8_t index;
    PWMOutput::GetTargetTemperature getTargetTemperature;
    
    void privateSetup();
    void loop();
    void readTemperature();
    void writePinDuty(float pwm);
    float getDuty() const;

#ifdef APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR
#if APB_PWM_OUTPUTS_SIZE > 0
#define __APB_PWM_OUTPUTS_PWM_PINOUT_INIT Pinout APB_PWM_OUTPUTS_PWM_PINOUT
#else
#define __APB_PWM_OUTPUTS_PWM_PINOUT_INIT
#endif

    struct Pinout {
        uint8_t pwm;
        int8_t thermistor;
        Type type = Heater;
    };
    static constexpr std::array<Pinout, APB_PWM_OUTPUTS_SIZE> pwmOutputsPinout{ __APB_PWM_OUTPUTS_PWM_PINOUT_INIT };
    int16_t pwmValue = -1;
    const Pinout *pinout = nullptr;
    NTC_Thermistor *ntcThermistor;
    std::unique_ptr<SmoothThermistor> smoothThermistor;
#endif

    static const std::unordered_map<Mode, const char*> modesToString;
    static const std::unordered_map<Type, const char*> typesToString;

};

const std::unordered_map<APB::PWMOutput::Mode, const char*> APB::PWMOutput::Private::modesToString = {
    { Mode::off, "off" },
    { Mode::dewpoint, "dewpoint" },
    { Mode::fixed, "fixed" },
    { Mode::target_temperature, "target_temperature" },
};

const std::unordered_map<APB::PWMOutput::Type, const char*> APB::PWMOutput::Private::typesToString = {
    { Type::Heater, "heater" },
    { Type::Output, "output" },
};



APB::PWMOutput::PWMOutput(Type type) : d{std::make_shared<Private>(this, type)} {
}

APB::PWMOutput::~PWMOutput() {
}

APB::PWMOutput::Type APB::PWMOutput::type() const {
    return d->type;
}

void APB::PWMOutput::setup(uint8_t index, Scheduler &scheduler) {
    d->index = index;
    sprintf(d->log_scope, "PWMOutput[%d] -", index);

    d->privateSetup();

    d->loopTask.set(APB_PWM_OUTPUT_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&PWMOutput::Private::loop, d));
    scheduler.addTask(d->loopTask);
    d->loopTask.enable();
    loadFromJson(); 
    Log.infoln("%s PWMOutput initialised", d->log_scope);
}


void APB::PWMOutput::loadFromJson() {
    if(LittleFS.exists(PWM_OUT_CONF_FILENAME)) {
        File file = LittleFS.open(PWM_OUT_CONF_FILENAME, "r");
        if(!file) {
            Log.errorln("%s Error opening pwmOutputs configuration file", d->log_scope);
            return;
        } else {
            Log.infoln("%s PWMOutputs configuration file opened", d->log_scope);
        }
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        if(error) {
            Log.errorln("%s Error parsing pwmOutputs configuration file: %s", d->log_scope, error.c_str());
            return;
        }
        JsonArray pwmOutputs = doc.as<JsonArray>();
        if(pwmOutputs.size() <= d->index) {
            Log.errorln("%s PWMOutputs configuration file doesn't have enough pwmOutputs", d->log_scope);
            return;
        }
        bool applyAtStartup = pwmOutputs[d->index]["apply_at_startup"].as<bool>();
        Log.infoln("%s PWMOutputs configuration file loaded, applyAtStartup=%T", d->log_scope, applyAtStartup);
        if(applyAtStartup) {
            d->readTemperature();
            const char *error = setState(pwmOutputs[d->index].as<JsonObject>());
            if(error) {
                Log.errorln("%s Error setting pwm output state from configuration: %s", d->log_scope, error);
            }
        }
    }
}


void APB::PWMOutput::toJson(JsonObject pwmOutputStatus) {
    pwmOutputStatus["mode"] = modeAsString(),
    pwmOutputStatus["max_duty"] = maxDuty();
    pwmOutputStatus["duty"] = duty();
    pwmOutputStatus["active"] = active();
    pwmOutputStatus["has_temperature"] = temperature().has_value();
    pwmOutputStatus["apply_at_startup"] = d->applyAtStartup;
    pwmOutputStatus["type"] = d->typesToString.at(type());
    optional::if_present(rampOffset(), [&](float v){ pwmOutputStatus["ramp_offset"] = v; });
    optional::if_present(minDuty(), [&](float v){ pwmOutputStatus["min_duty"] = v; });
    optional::if_present(temperature(), [&](float v){ pwmOutputStatus["temperature"] = v; });
    optional::if_present(targetTemperature(), [&](float v){ pwmOutputStatus["target_temperature"] = v; });
    optional::if_present(dewpointOffset(), [&](float v){ pwmOutputStatus["dewpoint_offset"] = v; });
}

std::forward_list<String> APB::PWMOutput::validModes()
{
    static std::forward_list<String> keys;
    if(keys.empty())
        std::transform(Private::modesToString.begin(), Private::modesToString.end(), std::front_inserter(keys), [](const auto &i) { return i.second; });
    return keys;
}

APB::PWMOutput::Mode APB::PWMOutput::modeFromString(const String &mode) {
    const auto found = std::find_if(Private::modesToString.begin(), Private::modesToString.end(), [&mode](const auto item){ return mode == item.second; });
    if(found == Private::modesToString.end()) {
        return Mode::off;
    }
    return found->first; 
}

const String APB::PWMOutput::modeAsString() const
{
    return Private::modesToString.at(mode());
}

float APB::PWMOutput::maxDuty() const {
    return d->maxDuty;
}

float APB::PWMOutput::duty() const {
    return d->getDuty();
}

bool APB::PWMOutput::active() const {
    return d->getDuty() > 0;
}

bool APB::PWMOutput::applyAtStartup() const {
    return d->applyAtStartup;
}

void APB::PWMOutput::setMaxDuty(float duty) {
    if(duty > 0) {
        d->maxDuty = duty;
        d->mode = PWMOutput::Mode::fixed;
    } else {
        d->mode = PWMOutput::Mode::off;
    }
    d->loop();
}

const char *APB::PWMOutput::setState(JsonObject json) {
    PWMOutput::Mode mode = PWMOutput::modeFromString(json["mode"]);
    d->applyAtStartup = json["apply_at_startup"].as<bool>();
    if(mode == PWMOutput::Mode::off) {
        setMaxDuty(0);
        return nullptr;
    }
    
    float duty = json["max_duty"];
    static const char *temperatureErrorMessage = "Unable to set target temperature. PWM output probably doesn't have a temperature sensor.";
    static const char *dewpointTemperatureErrorMessage = "Unable to set target temperature. Either the PWM output doesn't have a temperature sensor, or you're missing an ambient sensor.";

    if(mode == PWMOutput::Mode::fixed) {
        setMaxDuty(json["max_duty"]);
    }
    if(mode == PWMOutput::Mode::dewpoint) {
        float dewpointOffset = json["dewpoint_offset"];
        float minDuty = json["min_duty"].is<float>() ? json["min_duty"] : 0.f;
        float rampOffset = json["ramp_offset"].is<float>() ? json["ramp_offset"] : 0.f;
        if(!setDewpoint(dewpointOffset, duty, minDuty, rampOffset)) {
            return dewpointTemperatureErrorMessage;
        }
    }
    if(mode == PWMOutput::Mode::target_temperature) {
        float targetTemperature = json["target_temperature"];
        float rampOffset = json["ramp_offset"].is<float>() ? json["ramp_offset"] : 0.f;
        float minDuty = json["min_duty"].is<float>() ? json["min_duty"] : 0.f;
        if(!setTemperature(targetTemperature, duty, minDuty, rampOffset)) {
            return temperatureErrorMessage;
        }
    }
    return nullptr;
}

uint8_t APB::PWMOutput::index() const { 
    return d->index;
}


bool APB::PWMOutput::setTemperature(float targetTemperature, float maxDuty, float minDuty, float rampOffset) {
    if(!this->temperature().has_value()) {
        Log.warningln(TEMPERATURE_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    d->targetTemperature = targetTemperature;
    d->maxDuty = maxDuty;
    d->minDuty = minDuty;
    d->mode = PWMOutput::Mode::target_temperature;
    d->rampOffset = rampOffset >= 0 ? rampOffset : 0;
    d->loop();
    return true;
}

bool APB::PWMOutput::setDewpoint(float offset, float maxDuty, float minDuty, float rampOffset) {
    if(!this->temperature().has_value()) {
        Log.warningln(TEMPERATURE_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    if(!Ambient::Instance.reading().has_value()) {
        Log.warningln(AMBIENT_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    d->dewpointOffset = offset;
    d->rampOffset = rampOffset >= 0 ? rampOffset : 0;
    d->minDuty = minDuty;
    d->maxDuty = maxDuty;
    d->mode = PWMOutput::Mode::dewpoint;
    d->loop();
    return true;
}


std::optional<float> APB::PWMOutput::targetTemperature() const {
    if(d->mode != PWMOutput::Mode::target_temperature) {
        return {};
    }
    return {d->targetTemperature};
}

std::optional<float> APB::PWMOutput::dewpointOffset() const {
    if(d->mode != PWMOutput::Mode::dewpoint) {
        return {};
    }
    return {d->dewpointOffset};
}

std::optional<float> APB::PWMOutput::rampOffset() const {
    if(d->mode != Mode::dewpoint && d->mode != Mode::target_temperature) {
        return {};
    }
    return {d->rampOffset};
}

std::optional<float> APB::PWMOutput::minDuty() const {
    if(d->mode != Mode::dewpoint && d->mode != Mode::target_temperature) {
        return {};
    }
    return {d->minDuty};
}

std::optional<float> APB::PWMOutput::temperature() const {
    return d->temperature;
}

APB::PWMOutput::Mode APB::PWMOutput::mode() const {
    return d->mode;
}

void APB::PWMOutput::Private::loop()
{
    readTemperature();
    
    if(temperature.has_value() && temperature.value() < -50) {
        #ifdef DEBUG_HEATER_STATUS
        Log.traceln("%s invalid temperature detected, discarding temperature", log_scope);
        #endif
        temperature = {};
        if(mode == PWMOutput::Mode::dewpoint || mode == PWMOutput::Mode::target_temperature) {
            Log.warningln("%s Lost temperature sensor, switching off.", log_scope);
            mode = PWMOutput::Mode::off;
        }
    }

    if(mode == PWMOutput::Mode::fixed) {
        writePinDuty(maxDuty);
        return;
    }
    if(mode == PWMOutput::Mode::off) {
        writePinDuty(0);
        return;
    }
    // From nmow on we require a temperature sensor on the heater
    if(!temperature) {
        Log.warningln("%s Unable to set target temperature, sensor not found.", log_scope);
        q->setMaxDuty(0);
        return;
    }

    float dynamicTargetTemperature;
    if(mode == PWMOutput::Mode::target_temperature) {
        dynamicTargetTemperature = this->targetTemperature;
    }
    if(mode == PWMOutput::Mode::dewpoint) {
        if(!Ambient::Instance.reading()) {
            Log.warningln("%s Unable to set target temperature, ambient sensor not found.", log_scope);
            q->setMaxDuty(0);
            return;
        }
        dynamicTargetTemperature = dewpointOffset + Ambient::Instance.reading()->dewpoint();
    }

    float currentTemperature = temperature.value();
    Log.traceln("%s Got target temperature=`%F`", log_scope, dynamicTargetTemperature);
    Log.traceln("%s current temperature=`%F`", log_scope, currentTemperature);
    if(currentTemperature < dynamicTargetTemperature) {
        float rampFactor = rampOffset > 0 ? (dynamicTargetTemperature - currentTemperature)/rampOffset : 1;
        float targetPWM = std::max(0.f, std::min(1.f, rampFactor * (maxDuty-minDuty) + minDuty));
        Log.infoln("%s - temperature `%F` lower than target temperature `%F`, ramp=`%F` and PWM range is `%F-%F`, ramp factor=`%F`, setting PWM to `%F`",
            log_scope,
            currentTemperature,
            dynamicTargetTemperature,
            rampOffset,
            minDuty,
            maxDuty,
            rampFactor,
            targetPWM
        );
        writePinDuty(targetPWM);
    } else {
        Log.infoln("%s - temperature `%F` reached target temperature `%F`, setting PWM to 0", log_scope, currentTemperature, dynamicTargetTemperature);
        writePinDuty(0);
    }
}

#ifdef APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR
#define ANALOG_READ_RES 12
#define MAX_PWM 255.0
void APB::PWMOutput::Private::privateSetup() {
    static const float analogReadMax = std::pow(2, ANALOG_READ_RES) - 1;
    pinout = &pwmOutputsPinout[index];
    type = pinout->type;
    Log.traceln("%s Configuring PWM thermistor heater: Thermistor pin=%d, PWM pin=%d, analogReadMax=%F", log_scope, pinout->thermistor, pinout->pwm, analogReadMax);
    analogReadResolution(ANALOG_READ_RES);
    writePinDuty(0);
    if(pinout->thermistor != -1) {
        smoothThermistor = std::make_unique<SmoothThermistor>(
            pinout->thermistor,
            ANALOG_READ_RES,
            APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR_NOMINAL,
            APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR_REFERENCE,
            APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR_B_VALUE,
            APB_PWM_OUTPUT_TEMPERATURE_SENSOR_THERMISTOR_NOMINAL_TEMP,
            APB_PWM_OUTPUT_TEMPERATURE_AVERAGE_COUNT
        );
        Log.traceln("%s Created SmoothThermistor instance, initial readout=%F", log_scope, smoothThermistor->temperature());
    }
}

void APB::PWMOutput::Private::readTemperature() {
    if(!smoothThermistor) {
        return;
    }
    auto rawValue = analogRead(pinout->thermistor);
    temperature = smoothThermistor->temperature();
    #ifdef DEBUG_HEATER_STATUS
    Log.infoln("%s readThemperature: raw=%F, from smoothThermistor: %F", log_scope, rawValue, *temperature);
    #endif
}

float APB::PWMOutput::Private::getDuty() const {
    int8_t pwmChannel = analogGetChannel(pinout->thermistor);
    float pwmValue = ledcRead(pwmChannel);
    // Log.traceln("%s PWM value from ADC channel %d: %F; stored PWM value: %d", log_scope, pwmChannel, pwmValue, this->pwmValue);
    return this->pwmValue/MAX_PWM;
}

void APB::PWMOutput::Private::writePinDuty(float pwm) {
    int16_t newPWMValue = std::max(int16_t{0}, static_cast<int16_t>(std::min(MAX_PWM, MAX_PWM * pwm)));
    if(newPWMValue != pwmValue) {
        pwmValue = newPWMValue;
        Log.traceln("%s setting PWM=%d for pin %d", log_scope, pwmValue, pinout->pwm);
        analogWrite(pinout->pwm, pwmValue);
    }
}

#endif

void APB::PWMOutputs::toJson(JsonArray pwmOutputStatus) {
    std::for_each(PWMOutputs::Instance.begin(), PWMOutputs::Instance.end(), [pwmOutputStatus](PWMOutput &pwmOutput) {
        JsonObject pwmOutputObject = pwmOutputStatus[pwmOutput.index()].to<JsonObject>();
        pwmOutput.toJson(pwmOutputObject);
    });
}

void APB::PWMOutputs::saveConfig() {
    Log.infoln("[PWMOutputs] Saving pwmOutputs configuration");
    LittleFS.mkdir(APB_CONFIG_DIRECTORY);
    File file = LittleFS.open(PWM_OUT_CONF_FILENAME, "w",true);
    if(!file) {
        Log.errorln("[PWMOutputs] Error opening pwmOutputs configuration file" );
        return;
    }
    JsonDocument doc;
    toJson(doc.to<JsonArray>());
    serializeJson(doc, file);
    file.close();
    Log.infoln("[PWMOutputs] PWMOutputs configuration saved");
}

