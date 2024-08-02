#include <ArduinoLog.h>
#include <array>

#include "heater.h"
#include "configuration.h"

#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>
#endif


static const char *TEMPERATURE_NOT_FOUND_WARNING_LOG = "%s Cannot set heater temperature without temperature sensor";
static const char *AMBIENT_NOT_FOUND_WARNING_LOG = "%s Cannot set heater temperature without ambient sensor";

struct APB::Heater::Private {
    APB::Heater *q;
    Heater::Mode mode{Heater::Mode::off};
    float pwm;
    std::optional<float> temperature;
    float targetTemperature;
    float dewpointOffset;

    Task loopTask;
    char log_scope[20];
    uint8_t index;
    Heater::GetTargetTemperature getTargetTemperature;
    Ambient *ambient;
    
    void setup();
    void loop();
    void readTemperature();
    void setDuty(float pwm);
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
};

APB::Heater::Heater() : d{std::make_shared<Private>()} {
    d->q = this;
}

APB::Heater::~Heater() {
}

void APB::Heater::setup(uint8_t index, Scheduler &scheduler, Ambient *ambient) {
    d->index = index;
    sprintf(d->log_scope, "Heater[%d] -", index);

    d->setup();

    d->loopTask.set(APB_HEATER_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Heater::Private::loop, d));
    scheduler.addTask(d->loopTask);
    d->loopTask.enable();
    
    d->ambient = ambient;
    Log.infoln("%s Heater initialised", d->log_scope);

}

float APB::Heater::duty() const {
    return d->pwm;
}

bool APB::Heater::active() const {
    return d->getDuty() > 0;
}

void APB::Heater::setDuty(float duty) {
    if(duty > 0) {
        d->pwm = duty;
        d->mode = Heater::Mode::fixed;
    } else {
        d->mode = Heater::Mode::off;
    }
    d->loop();
}

uint8_t APB::Heater::index() const { 
    return d->index;
}


bool APB::Heater::setTemperature(float targetTemperature, float maxDuty) {
    if(!this->temperature().has_value()) {
        Log.warningln(TEMPERATURE_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    d->targetTemperature = targetTemperature;
    d->pwm = maxDuty;
    d->mode = Heater::Mode::target_temperature;
    d->loop();
    return true;
}

bool APB::Heater::setDewpoint(float offset, float maxDuty) {
    if(!this->temperature().has_value()) {
        Log.warningln(TEMPERATURE_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    if(!d->ambient->reading().has_value()) {
        Log.warningln(AMBIENT_NOT_FOUND_WARNING_LOG, d->log_scope);
        return false;
    }
    d->dewpointOffset = offset;
    d->pwm = maxDuty;
    d->mode = Heater::Mode::dewpoint;
    d->loop();
    return true;
}

std::optional<float> APB::Heater::targetTemperature() const {
    if(d->mode != +Heater::Mode::target_temperature) {
        return {};
    }
    return {d->targetTemperature};
}

std::optional<float> APB::Heater::dewpointOffset() const {
    if(d->mode != +Heater::Mode::dewpoint) {
        return {};
    }
    return {d->dewpointOffset};
}

std::optional<float> APB::Heater::temperature() const {
    return d->temperature;
}

APB::Heater::Mode APB::Heater::mode() const {
    return d->mode;
}

void APB::Heater::Private::loop()
{
    if(smoothThermistor) {
        readTemperature();
    }
    
    if(temperature.has_value() && temperature.value() < -50) {
        Log.traceln("%s invalid temperature detected, discarding temperature", log_scope);
        temperature = {};
        if(mode == +Heater::Mode::dewpoint || mode == +Heater::Mode::target_temperature) {
            Log.warningln("%s Lost temperature sensor, switching off.", log_scope);
            mode = Heater::Mode::off;
        }
    }

    if(mode == +Heater::Mode::fixed) {
        setDuty(pwm);
        return;
    }
    if(mode == +Heater::Mode::off) {
        setDuty(0);
        return;
    }
    // From nmow on we require a temperature sensor on the heater
    if(!temperature) {
        Log.warningln("%s Unable to set target temperature, sensor not found.", log_scope);
        q->setDuty(0);
        return;
    }

    float targetTemperature;
    if(mode == +Heater::Mode::target_temperature) {
        targetTemperature = this->targetTemperature;
    }
    if(mode == +Heater::Mode::dewpoint) {
        if(!ambient->reading()) {
            Log.warningln("%s Unable to set target temperature, ambient sensor not found.", log_scope);
            q->setDuty(0);
            return;
        }
        targetTemperature = dewpointOffset + ambient->reading()->dewpoint();
    }

    float currentTemperature = temperature.value();
    Log.traceln("%s Got target temperature=`%F`", log_scope, targetTemperature);
    Log.traceln("%s current temperature=`%F`", log_scope, currentTemperature);
    if(currentTemperature < targetTemperature) {
        Log.infoln("%s - temperature `%F` lower than target temperature `%F`, setting PWM to `%F`", log_scope, currentTemperature, targetTemperature, pwm);
        setDuty(pwm);
    } else {
        Log.infoln("%s - temperature `%F` reached target temperature `%F`, setting PWM to 0", log_scope, currentTemperature, targetTemperature);
        setDuty(0);
    }
}

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
#include <esp_random.h>

void APB::Heater::Private::setup() {
}

void APB::Heater::setSimulation(const std::optional<float> &temperature) {
    d->temperature = temperature;
}

void APB::Heater::Private::readTemperature() {
    float delta = static_cast<double>(esp_random())/UINT32_MAX;
    if(temperature.has_value()) {
        *temperature += delta - 0.5;
        Log.traceln("%s Heater simulator: loop, temp_diff=%F, temperature=%F", log_scope, delta, *temperature);
    }
}

void APB::Heater::Private::setPWM(float pwm) {
    Log.traceln("%s SIM: Setting PWM to %F", log_scope, pwm);
}

float APB::Heater::Private::getPWM() const {
    return pwm;
}
#endif


#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
#define ANALOG_READ_RES 12
#define MAX_PWM 255.0
void APB::Heater::Private::setup() {
    static const float analogReadMax = std::pow(2, ANALOG_READ_RES) - 1;
    pinout = &heaters_pinout[index];
    Log.traceln("%s Configuring PWM thermistor heater: Thermistor pin=%d, PWM pin=%d, analogReadMax=%F", log_scope, pinout->thermistor, pinout->pwm, analogReadMax);
    analogReadResolution(ANALOG_READ_RES);
    setDuty(0);
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
    auto rawValue = analogRead(pinout->thermistor);
    Log.infoln("Thermistor %d raw value for pin %d: %d", index, pinout->thermistor, rawValue);
    temperature = smoothThermistor->temperature();
    Log.traceln("%s readThemperature from smoothThermistor: %F", log_scope, *temperature);
}

float APB::Heater::Private::getDuty() const {
    int8_t pwmChannel = analogGetChannel(pinout->thermistor);
    float pwmValue = ledcRead(pwmChannel);
    // Log.traceln("%s PWM value from ADC channel %d: %F; stored PWM value: %d", log_scope, pwmChannel, pwmValue, this->pwmValue);
    return this->pwmValue/MAX_PWM;
}

void APB::Heater::Private::setDuty(float pwm) {
    int16_t newPWMValue = MAX_PWM * pwm;
    if(newPWMValue != pwmValue) {
        pwmValue = newPWMValue;
        Log.traceln("%s setting PWM=%d for pin %d", log_scope, pwmValue, pinout->pwm);
        analogWrite(pinout->pwm, pwmValue);
    }
}

#endif