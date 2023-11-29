#include <Arduino.h>
#include <ElegantOTA.h>
#include "webserver.h"
#include "wifimanager.h"
#include "settings.h"
#include <logger.h>

logging::Logger logger;

APB::Settings configuration(logger);
APB::WiFiManager wifiManager(configuration, logger);
APB::WebServer webServer(logger, configuration, wifiManager);

#define LOG_SCOPE F("APB::Main")
void setup() {
  Serial.begin(115200);
  // while(!Serial)
  sleep(5);
  logger.setDebugLevel(logging::LoggerLevel::LOGGER_LEVEL_DEBUG);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "AstroPowerBox::setup");
  

  configuration.setup();
  wifiManager.setup();
  webServer.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
}
