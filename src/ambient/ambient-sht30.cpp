#include "ambient-p.h"
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SHT30

#include <SHT85.h>
#include <unordered_map>

namespace {
SHT30 sht = SHT30{APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS};
void logSHT30Error(const char *phase) {
  static const std::unordered_map<uint8_t, String> SHT_ERRORS {
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
  int errorCode = sht.getError();
  String errorMessage = "Unknown error";
  if(SHT_ERRORS.count(errorCode)>0) {
    errorMessage = SHT_ERRORS.at(errorCode);
  }
  Log.errorln(LOG_SCOPE "SHT3x error (%s): %s [%d]", phase, errorMessage.c_str(), errorCode);
}
}

bool APB::Ambient::initialiseSensor() {
  Log.infoln(LOG_SCOPE "Ambient sensor: SHT%d at address 0x%x", sht.getType(), APB_AMBIENT_TEMPERATURE_SENSOR_I2C_ADDRESS);
  if(!sht.begin()) {
    logSHT30Error("initialiseSensor");
    return false;
  }
  return true;
}

void APB::Ambient::readSensor() {
  if(sht.read()) {
    _reading = { sht.getTemperature(), sht.getHumidity() };
    // Log.traceln("reading values: %d degrees, %d humidity", _reading->temperature, _reading->humidity);
  } else {
    logSHT30Error("reading values");
    _reading.reset();
  }
}

#endif

