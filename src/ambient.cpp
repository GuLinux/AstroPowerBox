#include "ambient.h"

APB::Ambient::Ambient(logging::Logger &logger) : logger{logger} {
}

float APB::Ambient::dewpoint() const {
    static const float dewpointA = 17.62;
    static const float dewpointB = 243.12;
    const float a_t_rh = log(humidity() / 100.0) + (dewpointA * temperature() / (dewpointB + temperature()));
    return (dewpointB * a_t_rh) / (dewpointA - a_t_rh);
}

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
namespace {
struct __AmbientPrivate {
    float temperature = 22;
    float humidity = 50;
};
__AmbientPrivate d;
}

void APB::Ambient::setSim(float temperature, float humidity) {
    d.temperature = temperature;
    d.humidity = humidity;
}

void APB::Ambient::setup() {
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Ambient", "Ambient simulator initialised");
}

float APB::Ambient::temperature() const {
    return d.temperature;
}

float APB::Ambient::humidity() const {
    return d.humidity;
}

#endif
