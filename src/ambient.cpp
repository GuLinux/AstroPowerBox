#include "ambient.h"

#include <ArduinoLog.h>

#if defined(APB_AMBIENT_TEMPERATURE_SENSOR_SHT30) 
#include <SHT85.h>
#include <map>
#endif

#define LOG_SCOPE "Ambient - "

struct APB::Ambient::Private {
  Task readValuesTask;
  bool initialised = false;
  bool initialiseSensor();
  void readSensor();
  std::optional<Reading> reading;
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30
  SHT30 sht = SHT30{APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS};
  void logSHT30Error(const char *phase);
#endif

};
namespace {
APB::Ambient::Private d;
}

APB::Ambient::Ambient() {
}


void APB::Ambient::setup(Scheduler &scheduler) {
  Log.infoln(LOG_SCOPE "Ambient initialising");
  d.initialised = d.initialiseSensor();
  if(d.initialised) {
    d.readValuesTask.set(APB_AMBIENT_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Ambient::Private::readSensor, &d));
    scheduler.addTask(d.readValuesTask);
    d.readValuesTask.enable();
    Log.infoln(LOG_SCOPE "Ambient initialised");
  } else {
    Log.errorln(LOG_SCOPE "Error initialising ambient sensor");
  }
}

bool APB::Ambient::isInitialised() const {
  return d.initialised;
}

std::optional<APB::Ambient::Reading> APB::Ambient::reading() const {
    return d.reading;
}

float APB::Ambient::Reading::dewpoint() const {
  static const float dewpointA = 17.62;
  static const float dewpointB = 243.12;
  const float a_t_rh = log(humidity / 100.0) + (dewpointA * temperature / (dewpointB + temperature));
  return (dewpointB * a_t_rh) / (dewpointA - a_t_rh);
}


#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30

bool APB::Ambient::Private::initialiseSensor() {
  Log.infoln(LOG_SCOPE "Ambient sensor: SHT type %d at address 0x%x", sht.getType(), APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS);
  if(!sht.begin()) {
    d.logSHT30Error("initialiseSensor");
    return false;
  }
  return true;
}

void APB::Ambient::Private::readSensor() {
  if(!d.initialised) return;
  if(d.sht.read()) {
    d.reading = { d.sht.getTemperature(), d.sht.getHumidity() };
  } else {
    d.logSHT30Error("reading values");
  }
}

void APB::Ambient::Private::logSHT30Error(const char *phase) {
  static const std::map<uint8_t, String> SHT_ERRORS {
    { SHT_OK, "no error" },
    { SHT_ERR_WRITECMD, "I2C write failed" },
    { SHT_ERR_READBYTES, "I2C read failed" },
    { SHT_ERR_HEATER_OFF, "Could not switch off heater" },
    { SHT_ERR_NOT_CONNECT, "Could not connect" },
    { SHT_ERR_CRC_TEMP, "CRC error in temperature" },
    { SHT_ERR_CRC_HUM, "CRC error in humidity" },
    { SHT_ERR_CRC_STATUS, "CRC error in status field" },
    { SHT_ERR_HEATER_COOLDOWN, "Heater need to cool down" },
    { SHT_ERR_HEATER_ON, "Could not switch on heater" },
  };
  int errorCode = d.sht.getError();
  String errorMessage = "Unknown error";
  if(SHT_ERRORS.count(errorCode)>0) {
    errorMessage = SHT_ERRORS.at(errorCode);
  }
  Log.errorln(LOG_SCOPE "SHT3x error (%s): %s [%d]", phase, errorMessage.c_str(), errorCode);
}

#endif
