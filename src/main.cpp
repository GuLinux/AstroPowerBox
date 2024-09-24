#include <Arduino.h>
#include <ElegantOTA.h>
#include <TaskScheduler.h>
#include <ArduinoLog.h>

#include "configuration.h"

#include "webserver.h"
#include "wifimanager.h"
#include "settings.h"
#include "ambient/ambient.h"
#include "heater.h"
#include "powermonitor.h"
#include <Wire.h>
#include <LittleFS.h>
#include <OneButton.h>
#include "statusled.h"
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include "asyncbufferedtcplogger.h"
#include "influxdb.h"

Scheduler scheduler;
AsyncServer loggerServer{9911};


APB::AsyncBufferedTCPLogger bufferedLogger{loggerServer};

#ifdef ONEBUTTON_USER_BUTTON_1
OneButton userButton;
#endif


APB::WebServer webServer(scheduler);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;

void setupArduinoOTA();

void setup() {
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
  auto wait_until = millis() + WAIT_FOR_SERIAL; while(!Serial && millis() < wait_until);
  #endif
  #ifdef BOOT_DELAY
  delay(BOOT_DELAY);
  #endif

  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.infoln(LOG_SCOPE "setup, core: %d", xPortGetCoreID());
  
  Log.addHandler(&bufferedLogger);

  LittleFS.begin();
  APB::Settings::Instance.setup();
  APB::InfluxDb::Instance.setup(scheduler);
  
  APB::StatusLed::Instance.setup();
  
  APB::WiFiManager::Instance.setup(scheduler);
  loggerServer.begin();
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  APB::Ambient::Instance.setup(scheduler);
  APB::PowerMonitor::Instance.setup(scheduler);
  std::for_each(APB::Heaters::Instance.begin(), APB::Heaters::Instance.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler); });
  
  webServer.setup();
  setupArduinoOTA();
  APB::History::Instance.setup(scheduler);

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.attachDoubleClick([]() {
    Log.infoln("[OneButton] User button 1 double clicked, reconnecting WiFi");
    APB::WiFiManager::Instance.reconnect();
  });
  userButton.setup(ONEBUTTON_USER_BUTTON_1, INPUT, false);
#endif
}

void loop() {
  APB::WiFiManager::Instance.loop(); 
  scheduler.execute();
  ElegantOTA.loop();

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.tick();
#endif
  ArduinoOTA.handle();
}

void setupArduinoOTA() {
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Log.infoln("Start updating %s", type.c_str());
      LittleFS.end();
    })
    .onEnd([]() {
      Log.infoln("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Log.infoln("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      String errorMessage;
      if (error == OTA_AUTH_ERROR) {
        errorMessage = "Auth Failed";
      } else if (error == OTA_BEGIN_ERROR) {
        errorMessage = "Begin Failed";
      } else if (error == OTA_CONNECT_ERROR) {
        errorMessage = "Connect Failed";
      } else if (error == OTA_RECEIVE_ERROR) {
        errorMessage = "Receive Failed";
      } else if (error == OTA_END_ERROR) {
        errorMessage = "End Failed";
      } else {
        errorMessage = "Unknown error";
      }
      Log.errorln("Error[%u]: %s", error, errorMessage.c_str());
    });

  ArduinoOTA.begin();
}