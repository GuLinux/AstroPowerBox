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
#include "powermonitor.h"
#include <Wire.h>
#include <LittleFS.h>
#include "statusled.h"

Scheduler scheduler;
APB::Settings settings;
APB::StatusLed led{settings};
APB::WiFiManager wifiManager{settings, led};
APB::Ambient ambient;
APB::Heaters heaters;
APB::PowerMonitor powerMonitor;


APB::WebServer webServer(settings, wifiManager, ambient, heaters, powerMonitor, scheduler, led);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;


void setup() {
  settings.setup();
  led.setup();
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
  auto started = millis(); while(!Serial && millis() - started < 10000);
  #endif
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.infoln(LOG_SCOPE "setup, core: %d", xPortGetCoreID());
  
  LittleFS.begin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  ambient.setup(scheduler);
  powerMonitor.setup(scheduler);
  std::for_each(heaters.begin(), heaters.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler, &ambient); });
  wifiManager.setup();
  webServer.setup();
}

void loop() {
  wifiManager.loop(); 
  scheduler.execute();
  ElegantOTA.loop();
}
