#include <Arduino.h>
#include <ElegantOTA.h>
#include <logger.h>
#include <TaskScheduler.h>

#include "configuration.h"

#include "webserver.h"
#include "wifimanager.h"
#include "settings.h"
#include "ambient.h"
#include "heater.h"


logging::Logger logger;
Scheduler scheduler;

APB::Settings configuration(logger);
APB::WiFiManager wifiManager(configuration, logger);
APB::Ambient ambient(logger);
APB::Heaters heaters;
APB::WebServer webServer(logger, configuration, wifiManager, ambient, heaters);


#define LOG_SCOPE F("APB::Main")

using namespace std::placeholders;

void setup() {
  Serial.begin(115200);
  auto started = millis(); while(!Serial && millis() - started < 10000);
  logger.setDebugLevel(logging::LoggerLevel::LOGGER_LEVEL_DEBUG);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "AstroPowerBox::setup, core: %d", xPortGetCoreID());
  

  configuration.setup();
  
  ambient.setup(scheduler);
  std::for_each(heaters.begin(), heaters.end(), [i=0](APB::Heater &heater) mutable { heater.setup(logger, i++, scheduler); });
  wifiManager.setup();
  webServer.setup();
}

void loop() {
  scheduler.execute();
}
