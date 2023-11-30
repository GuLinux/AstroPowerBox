#include <ArduinoLog.h>

#include "heater.h"
#include "configuration.h"


struct APB::Heater::Private {
    APB::Heater::Mode mode{APB::Heater::Mode::Off};
    float pwm = 0;
    std::optional<float> temperature;
    Task loopTask;
    char log_scope[20];
    uint8_t index;
    Heater::GetTargetTemperature getTargetTemperature;
    
    void loop();
    void readTemperature();
    void setPWM(float pwm);

#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
    struct Pinout {
        uint8_t pwm;
        uint8_t thermistor;
    }
    static constexpr std::array<Pinout, APB_HEATERS_SIZE>heaters_pinout{APB_HEATERS_PWM_PINOUT};
#endif
};

APB::Heater::Heater() : d{std::make_shared<Private>()} {
}

APB::Heater::~Heater() {
}

void APB::Heater::setup(uint8_t index, Scheduler &scheduler) {
    d->index = index;
    sprintf(d->log_scope, "Heater[%d] -", index);

    d->loopTask.set(APB_HEATER_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Heater::Private::loop, d));
    scheduler.addTask(d->loopTask);
    d->loopTask.enable();

    Log.infoln("%s Heater initialised", d->log_scope);

}

float APB::Heater::pwm() const {
    return d->pwm;
}

void APB::Heater::setPWM(float duty) {
    if(duty > 0) {
        d->pwm = duty;
        d->mode = APB::Heater::Mode::FixedPWM;
    } else {
        d->mode = APB::Heater::Mode::Off;
    }
}

uint8_t APB::Heater::index() const { 
    return d->index;
}

bool APB::Heater::setTemperature(GetTargetTemperature getTargetTemperature, float maxDuty) {
    if(!this->temperature().has_value()) {
        Log.warningln("%s Cannot set heater temperature without temperature sensor", d->log_scope);
        return false;
    }
    d->getTargetTemperature = getTargetTemperature;
    d->pwm = maxDuty;
    d->mode = APB::Heater::Mode::SetTemperature;
    return true;
}

std::optional<float> APB::Heater::temperature() const {
    return d->temperature;
}

APB::Heater::Mode APB::Heater::mode() const {
    return d->mode;
}

void APB::Heater::Private::loop() {
    readTemperature();
    if(temperature.has_value() && temperature.value() < -100) {
        Log.traceln("%s invalid temperature detected, discarding temperature");
        temperature = {};
    }
    if(mode._to_integral() == Heater::Mode::SetTemperature) {
        if(!temperature) {
            Log.warningln("%s Unable to set target temperature, sensor not found.", log_scope);
            setPWM(0);
            return;
        }
        float targetTemperature = getTargetTemperature();
        float currentTemperature = temperature.value();
        Log.traceln("%s Got target temperature=`%F`", log_scope, targetTemperature);
        Log.traceln("%s current temperature=`%F`", log_scope, currentTemperature);
        if(currentTemperature < targetTemperature) {
            Log.infoln("%s - temperature `%F` lower than target temperature `%F`, setting PWM to `%F`", log_scope, currentTemperature, targetTemperature, pwm);
            setPWM(pwm);
        } else {
            Log.infoln("%s - temperature `%F` reached target temperature `%F`, setting PWM to 0", log_scope, currentTemperature, targetTemperature);
            setPWM(0);
        }
    }
    if(mode._to_integral() == Heater::Mode::FixedPWM) {
        setPWM(pwm);
    }
    if(mode._to_integral() == Heater::Mode::Off) {
        setPWM(0);
    }
}

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
#include <esp_random.h>

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
#endif