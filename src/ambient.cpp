#include "ambient.h"

#define LOG_SCOPE "Ambient"

namespace {
struct __AmbientPrivate {
    float temperature = 0;
    float humidity = 0;
};
__AmbientPrivate d;
}



APB::Ambient::Ambient(logging::Logger &logger) : logger{logger} {
}

float APB::Ambient::dewpoint() const {
    static const float dewpointA = 17.62;
    static const float dewpointB = 243.12;
    const float a_t_rh = log(humidity() / 100.0) + (dewpointA * temperature() / (dewpointB + temperature()));
    return (dewpointB * a_t_rh) / (dewpointA - a_t_rh);
}

float APB::Ambient::temperature() const {
    return d.temperature;
}

float APB::Ambient::humidity() const {
    return d.humidity;
}

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
#include <esp_random.h>
void APB::Ambient::setSim(float temperature, float humidity) {
    d.temperature = temperature;
    d.humidity = humidity;
}

void APB::Ambient::readValues() {
  auto getRandomDelta = [](){
    float delta = static_cast<double>(esp_random())/UINT32_MAX;
    return delta - 0.5;
  };
  auto temp_delta = getRandomDelta();
  auto hum_delta = getRandomDelta();
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Ambient simulator: readValues, temp_diff=%f, hum_diff=%f", temp_delta, hum_delta);
  
  d.temperature += temp_delta;
  d.humidity += hum_delta;
  d.humidity = std::min(float(100.), std::max(float(0.), d.humidity));
}

void APB::Ambient::setup(Scheduler &scheduler) {
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "Ambient simulator initialised");
  readValuesTask.set(2000, TASK_FOREVER, std::bind(&Ambient::readValues, this));
  scheduler.addTask(readValuesTask);
  readValuesTask.enable();
}

#endif
