#include "ambient-p.h"
#ifdef APB_AMBIENT_TEMPERATURE_SENSOR_NONE



bool APB::Ambient::initialiseSensor() {
  Log.infoln(LOG_SCOPE "No Ambient sensor installed");
  return false;
}

void APB::Ambient::readSensor() {
}

#endif

