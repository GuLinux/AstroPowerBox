#include <ArduinoLog.h>
#include <LittleFS.h>
#include <array>

#include "heater.h"
#include "configuration.h"

#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>
#endif

#include "settings.h"
#include "utils.h"
#define HEATERS_CONF_FILENAME APB_CONFIG_DIRECTORY "/heaters.json"


static const char *TEMPERATURE_NOT_FOUND_WARNING_LOG = "%s Cannot set heater temperature without temperature sensor";
static const char *AMBIENT_NOT_FOUND_WARNING_LOG = "%s Cannot set heater temperature without ambient sensor";

APB::Heaters::Array &APB::Heaters::Instance = *new APB::Heaters::Array();

struct APB::Heater::Private {
    APB::Heater *q;
    Heater::Mode mode{Heater::Mode::off};
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
    Heater::GetTargetTemperature getTargetTemperature;
    
    void privateSetup();
    void loop();
    void readTemperature();
    void writePinDuty(float pwm);
    float getDuty() const;

#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
    struct Pinout {
        uint8_t pwm;
        int8_t thermistor;
    };
    static constexpr std::array<Pinout, APB_HEATERS_SIZE> heaters_pinout{ APB_HEATERS_PWM_PINOUT };
    int16_t pwmValue = -1;
    const Pinout *pinout = nullptr;
    NTC_Thermistor *ntcThermistor;
    std::unique_ptr<SmoothThermistor> smoothThermistor;
#endif

    static std::unordered_map<Heater::Mode, String> modesToString;
};

std::unordered_map<APB::Heater::Mode, String> APB::Heater::Private::modesToString = {
    { APB::Heater::Mode::off, "off" },
    { APB::Heater::Mode::dewpoint, "dewpoint" },
    { APB::Heater::Mode::fixed, "fixed" },
    { APB::Heater::Mode::target_temperature, "target_temperature" },
};



APB::Heater::Heater() : d{std::make_shared<Private>()} {
    d->q = this;
}

APB::Heater::~Heater() {
}

void APB::Heater::setup(uint8_t index, Scheduler &scheduler) {
    d->index = index;
    sprintf(d->log_scope, "Heater[%d] -", index);

    d->privateSetup();

    d->loopTask.set(APB_HEATER_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Heater::Private::loop, d));
    scheduler.addTask(d->loopTask);
    d->loopTask.enable();
    loadFromJson(); 
    Log.infoln("%s Heater initialised", d->log_scope);
}


void APB::Heater::loadFromJson() {
    if(LittleFS.exists(HEATERS_CONF_FILENAME)) {
        File file = LittleFS.open(HEATERS_CONF_FILENAME, "r");
        if(!file) {
            Log.errorln("%s Error opening heaters configuration file", d->log_scope);
            return;
        } else {
            Log.infoln("%s Heaters configuration file opened", d->log_scope);
        }
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        if(error) {
            Log.errorln("%s Error parsing heaters configuration file: %s", d->log_scope, error.c_str());
            return;
        }
        JsonArray heaters = doc.as<JsonArray>();
        if(heaters.size() <= d->index) {
            Log.errorln("%s Heaters configuration file doesn't have enough heaters", d->log_scope);
            return;
        }
        bool applyAtStartup = heaters[d->index]["apply_at_startup"].as<bool>();
        Log.infoln("%s Heaters configuration file loaded, applyAtStartup=%T", d->log_scope, applyAtStartup);
        if(applyAtStartup) {
            d->readTemperature();
            const char *error = setState(heaters[d->index].as<JsonObject>());
            if(error) {
                Log.errorln("%s Error setting heater state from configuration: %s", d->log_scope, error);
            }
        }
    }
}


void APB::Heater::toJson(JsonObject heaterStatus) {
    heaterStatus["mode"] = modeAsString(),
    heaterStatus["max_duty"] = maxDuty();
    heaterStatus["duty"] = duty();
    heaterStatus["active"] = active();
    heaterStatus["has_temperature"] = temperature().has_value();
    heaterStatus["apply_at_startup"] = d->applyAtStartup;
    optional::if_present(rampOffset(), [&](float v){ heaterStatus["ramp_offset"] = v; });
    optional::if_present(minDuty(), [&](float v){ heaterStatus["min_duty"] = v; });
    optional::if_present(temperature(), [&](float v){ heaterStatus["temperature"] = v; });
    optional::if_present(targetTemperature(), [&](float v){ heaterStatus["target_temperature"] = v; });
    optional::if_present(dewpointOffset(), [&](float v){ heaterStatus["dewpoint_offset"] = v; });
}

std::forward_list<String> APB::Heater::validModes()
{
    static std::forward_list<String> keys;
    if(keys.empty())
        std::transform(Private::modesToString.begin(), Private::modesToString.end(), std::front_inserter(keys), [](const auto &i) { return i.second; });
    return keys;
}

APB::Heater::Mode APB::Heater::modeFromString(const String &mode) {
    const auto found = std::find_if(Private::modesToString.begin(), Private::modesToString.end(), [&mode](const auto item){ return mode == item.second; });
    if(found == Private::modesToString.end()) {
        return Mode::off;
    }
    return found->first; 
}

const String APB::Heater::modeAsString() const
{
    return Private::modesToString[mode()];
}

float APB::Heater::maxDuty() const {
    return d->maxDuty;
}

float APB::Heater::duty() const {
    return d->getDuty();
}

bool APB::Heater::active() const {
    return d->getDuty() > 0;
}

bool APB::Heater::applyAtStartup() const {
    return d->applyAtStartup;
}

void APB::Heater::setMaxDuty(float duty) {
    if(duty > 0) {
        d->maxDuty = duty;
        d->mode = Heater::Mode::fixed;
    } else {
        d->mode = Heater::Mode::off;
    }
    d->loop();
}

const char *APB::Heater::setState(JsonObject json) {
    Heater::Mode mode = Heater::modeFromString(json["mode"]);
    d->applyAtStartup = json["apply_at_startup"].as<bool>();
    if(mode == Heater::Mode::off) {
        setMaxDuty(0);
        return nullptr;
    }
    
    float duty = json["max_duty"];
    static const char *temperatureErrorMessage = "Unable to set target temperature. Heater probably doesn't have a temperature sensor.";
    static const char *dewpointTemperatureErrorMessage = "Unable to set target temperature. Either the heater doesn't have a temperature sensor, or you're missing an ambient sensor.";

    if(mode == Heater::Mode::fixed) {
        setMaxDuty(json["max_duty"]);
    }
    if(mode == Heater::Mode::dewpoint) {
        float dewpointOffset = json["dewpoint_offset"];
        float minDuty = json["min_duty"].is<float>() ? json["min_duty"] : 0.f;
        float rampOffset = json["ramp_offset"].is<float>() ? json["ramp_offset"] : 0.f;
        if(!setDewpoint(dewpointOffset, duty, minDuty, rampOffset)) {
            return dewpointTemperatureErrorMessage;
        }
    }
    if(mode == Heater::Mode::target_temperature) {
        float targetTemperature = json["target_temperature"];
        float rampOffset = json["ramp_offset"].is<float>() ? json["ramp_offset"] : 0.f;
        float minDuty = json["min_duty"].is<float>() ? json["min_duty"] : 0.f;
        if(!setTemperature(targetTemperature, duty, minDuty, rampOffset)) {
            return temperatureErrorMessage;
        }
    }
    return nullptr;
}

uint8_t APB::Heater::index() const { 
    return d->index;
}


bool APB::Heater::setTemperature(float targetTemperature, float maxDuty, float minDuty, float rampOffset) {
    if(!this->temperature().has_value()) {
        Log.warningln(TEMPERATURE_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    d->targetTemperature = targetTemperature;
    d->maxDuty = maxDuty;
    d->minDuty = minDuty;
    d->mode = Heater::Mode::target_temperature;
    d->rampOffset = rampOffset >= 0 ? rampOffset : 0;
    d->loop();
    return true;
}

bool APB::Heater::setDewpoint(float offset, float maxDuty, float minDuty, float rampOffset) {
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
    d->mode = Heater::Mode::dewpoint;
    d->loop();
    return true;
}


std::optional<float> APB::Heater::targetTemperature() const {
    if(d->mode != Heater::Mode::target_temperature) {
        return {};
    }
    return {d->targetTemperature};
}

std::optional<float> APB::Heater::dewpointOffset() const {
    if(d->mode != Heater::Mode::dewpoint) {
        return {};
    }
    return {d->dewpointOffset};
}

std::optional<float> APB::Heater::rampOffset() const {
    if(d->mode != Mode::dewpoint && d->mode != Mode::target_temperature) {
        return {};
    }
    return {d->rampOffset};
}

std::optional<float> APB::Heater::minDuty() const {
    if(d->mode != Mode::dewpoint && d->mode != Mode::target_temperature) {
        return {};
    }
    return {d->minDuty};
}

std::optional<float> APB::Heater::temperature() const {
    return d->temperature;
}

APB::Heater::Mode APB::Heater::mode() const {
    return d->mode;
}

void APB::Heater::Private::loop()
{
    readTemperature();
    
    if(temperature.has_value() && temperature.value() < -50) {
        #ifdef DEBUG_HEATER_STATUS
        Log.traceln("%s invalid temperature detected, discarding temperature", log_scope);
        #endif
        temperature = {};
        if(mode == Heater::Mode::dewpoint || mode == Heater::Mode::target_temperature) {
            Log.warningln("%s Lost temperature sensor, switching off.", log_scope);
            mode = Heater::Mode::off;
        }
    }

    if(mode == Heater::Mode::fixed) {
        writePinDuty(maxDuty);
        return;
    }
    if(mode == Heater::Mode::off) {
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
    if(mode == Heater::Mode::target_temperature) {
        dynamicTargetTemperature = this->targetTemperature;
    }
    if(mode == Heater::Mode::dewpoint) {
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

#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
#define ANALOG_READ_RES 12
#define MAX_PWM 255.0
void APB::Heater::Private::privateSetup() {
    static const float analogReadMax = std::pow(2, ANALOG_READ_RES) - 1;
    pinout = &heaters_pinout[index];
    Log.traceln("%s Configuring PWM thermistor heater: Thermistor pin=%d, PWM pin=%d, analogReadMax=%F", log_scope, pinout->thermistor, pinout->pwm, analogReadMax);
    analogReadResolution(ANALOG_READ_RES);
    writePinDuty(0);
    if(pinout->thermistor != -1) {
        smoothThermistor = std::make_unique<SmoothThermistor>(
            pinout->thermistor,
            ANALOG_READ_RES,
            APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_NOMINAL,
            APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_REFERENCE,
            APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_B_VALUE,
            APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_NOMINAL_TEMP,
            APB_HEATER_TEMPERATURE_AVERAGE_COUNT
        );
        Log.traceln("%s Created SmoothThermistor instance, initial readout=%F", log_scope, smoothThermistor->temperature());
    }
}

void APB::Heater::Private::readTemperature() {
    if(!smoothThermistor) {
        return;
    }
    auto rawValue = analogRead(pinout->thermistor);
    temperature = smoothThermistor->temperature();
    #ifdef DEBUG_HEATER_STATUS
    Log.infoln("%s readThemperature: raw=%F, from smoothThermistor: %F", log_scope, rawValue, *temperature);
    #endif
}

float APB::Heater::Private::getDuty() const {
    int8_t pwmChannel = analogGetChannel(pinout->thermistor);
    float pwmValue = ledcRead(pwmChannel);
    // Log.traceln("%s PWM value from ADC channel %d: %F; stored PWM value: %d", log_scope, pwmChannel, pwmValue, this->pwmValue);
    return this->pwmValue/MAX_PWM;
}

void APB::Heater::Private::writePinDuty(float pwm) {
    int16_t newPWMValue = std::max(int16_t{0}, static_cast<int16_t>(std::min(MAX_PWM, MAX_PWM * pwm)));
    if(newPWMValue != pwmValue) {
        pwmValue = newPWMValue;
        Log.traceln("%s setting PWM=%d for pin %d", log_scope, pwmValue, pinout->pwm);
        analogWrite(pinout->pwm, pwmValue);
    }
}

#endif

void APB::Heaters::toJson(JsonArray heatersStatus) {
    std::for_each(Heaters::Instance.begin(), Heaters::Instance.end(), [heatersStatus](Heater &heater) {
        JsonObject heaterObject = heatersStatus[heater.index()].to<JsonObject>();
        heater.toJson(heaterObject);
    });
}

void APB::Heaters::saveConfig() {
    Log.infoln("[Heaters] Saving heaters configuration");
    LittleFS.mkdir(APB_CONFIG_DIRECTORY);
    File file = LittleFS.open(HEATERS_CONF_FILENAME, "w",true);
    if(!file) {
        Log.errorln("[Heaters] Error opening heaters configuration file" );
        return;
    }
    JsonDocument doc;
    toJson(doc.to<JsonArray>());
    serializeJson(doc, file);
    file.close();
    Log.infoln("[Heaters] Heaters configuration saved");
}

