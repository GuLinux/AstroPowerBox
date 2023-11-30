#include "heater.h"


namespace {
struct Heater_Private {
    APB::Heater::Mode mode{APB::Heater::Mode::Off};
    float pwm = 0;
    void readValues();
    void setPWM(float pwm);
};
    Heater_Private d;
}

void APB::Heater::setup(logging::Logger &logger, uint8_t index, Scheduler &scheduler) {
    this->logger = &logger;
    this->_index = index;
    sprintf(log_scope, "Heater[%d]", index);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, log_scope, "Heater initialised");
}

float APB::Heater::pwm() const {
    return d.pwm;
}

void APB::Heater::pwm(float duty) {
}

bool APB::Heater::temperature(float temperature, float maxDuty) {
    if(!this->temperature().has_value()) {
        logger->log(logging::LoggerLevel::LOGGER_LEVEL_WARN, log_scope, "Cannot set heater temperature without temperature sensor");
        return false;
    }
    return true;
}

std::optional<float> APB::Heater::temperature() const {
    return std::optional<float>();
}

APB::Heater::Mode APB::Heater::mode() const {
    return d.mode;
}
