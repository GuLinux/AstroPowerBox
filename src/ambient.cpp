#include "ambient.h"

#include <ArduinoLog.h>

#define LOG_SCOPE "Ambient - "


namespace {
struct __AmbientPrivate {
    float temperature = 0;
    float humidity = 0;
    Task readValuesTask;
};
__AmbientPrivate d;
}

APB::Ambient::Ambient() {
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

void APB::Ambient::setup(Scheduler &scheduler) {
  Log.infoln(LOG_SCOPE "Ambient simulator initialised");
  d.readValuesTask.set(APB_AMBIENT_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Ambient::readValues, this));
  scheduler.addTask(d.readValuesTask);
  d.readValuesTask.enable();
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
  Log.traceln(LOG_SCOPE "Ambient simulator: readValues, temp_diff=%F, hum_diff=%F", temp_delta, hum_delta);
  
  d.temperature += temp_delta;
  d.humidity += hum_delta;
  d.humidity = std::min(float(100.), std::max(float(0.), d.humidity));
}

#endif
