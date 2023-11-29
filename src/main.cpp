#include <Arduino.h>
#include <ElegantOTA.h>
#include <logger.h>

#include "webserver.h"
#include "wifimanager.h"
#include "settings.h"
#include "ambient.h"


logging::Logger logger;

APB::Settings configuration(logger);
APB::WiFiManager wifiManager(configuration, logger);
APB::Ambient ambient(logger);
APB::WebServer webServer(logger, configuration, wifiManager, ambient);

#define LOG_SCOPE F("APB::Main")
void setup() {
  Serial.begin(115200);
  // while(!Serial)
  sleep(5);
  logger.setDebugLevel(logging::LoggerLevel::LOGGER_LEVEL_DEBUG);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, LOG_SCOPE, "AstroPowerBox::setup");
  

  configuration.setup();
  wifiManager.setup();
  ambient.setup();
  webServer.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
}
