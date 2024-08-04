#include "ambient-p.h"
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT4x
#include <SensirionI2cSht4x.h>
#include <Wire.h>
#define SHT_NO_ERROR 0

namespace {
  SensirionI2cSht4x sht;
}

bool shtCheckForError(int16_t error, const char *errorPrefix) {
  if(error == SHT_NO_ERROR) {
    return false;
  }
  char errorMessage[64];
  errorToString(error, errorMessage, sizeof(errorMessage));
  Log.errorln("[Ambient-SHT4x] %s: %s", errorPrefix, errorMessage);
  return true;
}


bool APB::Ambient::initialiseSensor() {
  Log.infoln(LOG_SCOPE "Ambient sensor: SHT4x at address 0x%x", APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS);
  sht.begin(Wire, APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS);
  uint32_t serialNumber = 0;
  if(shtCheckForError(sht.serialNumber(serialNumber), "Error initialising SHT4x")) {
    return false;
  }

  Log.infoln(LOG_SCOPE "Ambient-SHT4x: sensor initialised, serial number: %d", serialNumber);
  return true;
}

void APB::Ambient::readSensor() {
  shtCheckForError(sht.measureHighPrecision(_reading->temperature, _reading->humidity), "Error reading temperature/humidity");
}

#endif

