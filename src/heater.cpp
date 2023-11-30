#include "heater.h"
#include "configuration.h"


namespace {
struct Heater_Private {
    APB::Heater::Mode mode{APB::Heater::Mode::Off};
    float pwm = 0;
    std::optional<float> temperature;
    Task loopTask;
    
    void loop();
    void setPWM(float pwm);

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
    void setSimulation(const std::optional<float> &temperature);
#endif
#ifdef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
    struct Pinout {
        uint8_t pwm;
        uint8_t thermistor;
    }
    static constexpr std::array<Pinout, APB_HEATERS_SIZE>heaters_pinout{APB_HEATERS_PWM_PINOUT};
#endif
};
    Heater_Private d;
}

void APB::Heater::setup(logging::Logger &logger, uint8_t index, Scheduler &scheduler) {
    this->logger = &logger;
    this->_index = index;
    sprintf(log_scope, "Heater[%d]", index);

    d.loopTask.set(APB_HEATER_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Heater_Private::loop, &d));
    scheduler.addTask(d.loopTask);
    d.loopTask.enable();

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, log_scope, "Heater initialised");

}

float APB::Heater::pwm() const {
    return d.pwm;
}

void APB::Heater::pwm(float duty) {
    d.setPWM(duty);
}

bool APB::Heater::temperature(float temperature, float maxDuty) {
    if(!this->temperature().has_value()) {
        logger->log(logging::LoggerLevel::LOGGER_LEVEL_WARN, log_scope, "Cannot set heater temperature without temperature sensor");
        return false;
    }
    return true;
}

std::optional<float> APB::Heater::temperature() const {
    return d.temperature;
}

APB::Heater::Mode APB::Heater::mode() const {
    return d.mode;
}

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM

void Heater_Private::setSimulation(const std::optional<float> &temperature) {

}

void Heater_Private::loop() {
}

void Heater_Private::setPWM(float pwm) {
    this->pwm = pwm;
}
#endif