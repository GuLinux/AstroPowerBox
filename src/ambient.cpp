#include "ambient.h"

#include <ArduinoLog.h>

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30
#include <SHT31.h>
#include <map>
#endif

#define LOG_SCOPE "Ambient - "


namespace {
struct __AmbientPrivate {
    float temperature;
    float humidity;
    Task readValuesTask;
    void setup();
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30
  SHT31 sht31;
  bool sensorInitialised = false;
  void logSHT31Error(const char *phase);
#endif
};
__AmbientPrivate d;
}

APB::Ambient::Ambient() {
}

float APB::Ambient::dewpoint() const {
    static const float dewpointA = 17.62;
    static const float dewpointB = 243.12;
    const float a_t_rh = log(d.humidity / 100.0) + (dewpointA * d.temperature / (dewpointB + d.temperature));
    return (dewpointB * a_t_rh) / (dewpointA - a_t_rh);
}

float APB::Ambient::temperature() const {
    return d.temperature;
}

float APB::Ambient::humidity() const {
    return d.humidity;
}

void APB::Ambient::setup(Scheduler &scheduler) {
  Log.infoln(LOG_SCOPE "Ambient initialising");
  d.setup();
  d.readValuesTask.set(APB_AMBIENT_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Ambient::readValues, this));
  scheduler.addTask(d.readValuesTask);
  d.readValuesTask.enable();
  Log.infoln(LOG_SCOPE "Ambient initialised");
}

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
#include <esp_random.h>
void __AmbientPrivate::setup() {
  Log.infoln(LOG_SCOPE "Ambient simulator initialised");
}
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

#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30

void __AmbientPrivate::setup() {
  sensorInitialised = sht31.begin(APB_AMBIENT_TEMPERATURE_SENSOR_SHT30_ADDRESS);
  if(!sensorInitialised) {
    d.logSHT31Error("setup");
  }
}
void APB::Ambient::readValues() {
  if(!d.sensorInitialised) return;
  if(d.sht31.readData()) {
    d.temperature = d.sht31.getTemperature();
    d.humidity = d.sht31.getHumidity();
  } else {
    d.logSHT31Error("reading values");
  }
}

void __AmbientPrivate::logSHT31Error(const char *phase) {
  static const std::map<uint8_t, String> SHT31_ERRORS {
    { SHT31_OK, "no error" },
    { SHT31_ERR_WRITECMD, "I2C write failed" },
    { SHT31_ERR_READBYTES, "I2C read failed" },
    { SHT31_ERR_HEATER_OFF, "Could not switch off heater" },
    { SHT31_ERR_NOT_CONNECT, "Could not connect" },
    { SHT31_ERR_CRC_TEMP, "CRC error in temperature" },
    { SHT31_ERR_CRC_HUM, "CRC error in humidity" },
    { SHT31_ERR_CRC_STATUS, "CRC error in status field" },
    { SHT31_ERR_HEATER_COOLDOWN, "Heater need to cool down" },
    { SHT31_ERR_HEATER_ON, "Could not switch on heater" },
  };
  int errorCode = d.sht31.getError();
  String errorMessage = "Unknown error";
  if(SHT31_ERRORS.count(errorCode)>0) {
    errorMessage = SHT31_ERRORS.at(errorCode);
  }
  Log.errorln(LOG_SCOPE "SHT3x error (%s): %s [%d]", phase, errorMessage.c_str(), errorCode);
}

#endif
