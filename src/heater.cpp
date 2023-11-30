#include "heater.h"
#include "configuration.h"


struct APB::Heater::Private {
    APB::Heater::Mode mode{APB::Heater::Mode::Off};
    float pwm = 0;
    std::optional<float> temperature;
    Task loopTask;
    logging::Logger *logger;
    char log_scope[20];
    uint8_t index;
    
    void loop();
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

void APB::Heater::setup(logging::Logger &logger, uint8_t index, Scheduler &scheduler) {
    d->logger = &logger;
    d->index = index;
    sprintf(d->log_scope, "Heater[%d]", index);

    d->loopTask.set(APB_HEATER_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Heater::Private::loop, d));
    scheduler.addTask(d->loopTask);
    d->loopTask.enable();

    d->logger->log(logging::LoggerLevel::LOGGER_LEVEL_INFO, d->log_scope, "Heater initialised");

}

float APB::Heater::pwm() const {
    return d->pwm;
}

void APB::Heater::pwm(float duty) {
    d->setPWM(duty);
}

uint8_t APB::Heater::index() const { 
    return d->index;
}

bool APB::Heater::temperature(float temperature, float maxDuty) {
    if(!this->temperature().has_value()) {
        d->logger->log(logging::LoggerLevel::LOGGER_LEVEL_WARN, d->log_scope, "Cannot set heater temperature without temperature sensor");
        return false;
    }
    return true;
}

std::optional<float> APB::Heater::temperature() const {
    return d->temperature;
}

APB::Heater::Mode APB::Heater::mode() const {
    return d->mode;
}

#ifdef APB_HEATER_TEMPERATURE_SENSOR_SIM
#include <esp_random.h>

void APB::Heater::setSimulation(const std::optional<float> &temperature) {
    d->temperature = temperature;
}

void APB::Heater::Private::loop() {
    float delta = static_cast<double>(esp_random())/UINT32_MAX;
    if(temperature.has_value()) {
        *temperature += delta - 0.5;
        logger->log(logging::LoggerLevel::LOGGER_LEVEL_INFO, log_scope, "Heater simulator: loop, temp_diff=%f, temperature=%f", delta, *temperature);
    }
}

void APB::Heater::Private::setPWM(float pwm) {
    this->pwm = pwm;
}
#endif