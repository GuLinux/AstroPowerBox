#include "ambient-p.h"


APB::Ambient::Ambient() {
}


void APB::Ambient::setup(Scheduler &scheduler) {
  Log.infoln(LOG_SCOPE "Ambient initialising");
  initialised = initialiseSensor();
  if(initialised) {
    readValuesTask.set(APB_AMBIENT_UPDATE_INTERVAL_SECONDS * 1000, TASK_FOREVER, std::bind(&Ambient::readSensor, this));
    scheduler.addTask(readValuesTask);
    readValuesTask.enable();
    Log.infoln(LOG_SCOPE "Ambient initialised");
  } else {
    Log.errorln(LOG_SCOPE "Error initialising ambient sensor");
  }
}

bool APB::Ambient::isInitialised() const {
  return initialised;
}


float APB::Ambient::Reading::dewpoint() const {
  static const float dewpointA = 17.62;
  static const float dewpointB = 243.12;
  const float a_t_rh = log(humidity / 100.0) + (dewpointA * temperature / (dewpointB + temperature));
  return (dewpointB * a_t_rh) / (dewpointA - a_t_rh);
}

