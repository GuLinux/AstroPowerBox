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
#include "bufferedlogger.h"
#include "async_led.h"

Scheduler scheduler;

AsyncLed led(APB_STATUS_LED_PIN, false);
APB::Settings configuration;
APB::WiFiManager wifiManager(configuration, led);
APB::Ambient ambient;
APB::Heaters heaters;
APB::PowerMonitor powerMonitor;


APB::WebServer webServer(configuration, wifiManager, ambient, heaters, powerMonitor, scheduler);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;


void setup() {
  AsyncLed::SetupLeds({&led});
  led.setDuty(0.3);
  led.setPattern(2, 2);
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
  auto started = millis(); while(!Serial && millis() - started < 10000);
  #endif
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.infoln(LOG_SCOPE "setup, core: %d", xPortGetCoreID());
  
  

  LittleFS.begin();

  configuration.setup();
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  ambient.setup(scheduler);
  powerMonitor.setup(scheduler);
  std::for_each(heaters.begin(), heaters.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler, &ambient); });
  led.setPattern(15, 5);
  wifiManager.setup();
  webServer.setup();
}

void loop() {
  wifiManager.loop(); 
  scheduler.execute();
  ElegantOTA.loop();
}
