#include <Arduino.h>
#include <ElegantOTA.h>
#include <TaskScheduler.h>
#include <ArduinoLog.h>

#include "configuration.h"

#include "webserver.h"
#include "wifimanager.h"
#include "settings.h"
#include "ambient.h"
#include "heater.h"


Scheduler scheduler;

APB::Settings configuration;
APB::WiFiManager wifiManager(configuration);
APB::Ambient ambient;
APB::Heaters heaters;

APB::WebServer webServer(configuration, wifiManager, ambient, heaters);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;

void setup() {
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
  auto started = millis(); while(!Serial && millis() - started < 10000);
  #endif
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.infoln(LOG_SCOPE "setup, core: %d", xPortGetCoreID());
  

  configuration.setup();
  
  ambient.setup(scheduler);
  std::for_each(heaters.begin(), heaters.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler); });
  wifiManager.setup();
  webServer.setup();
}

void loop() {
  scheduler.execute();
}
