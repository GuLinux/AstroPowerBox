#include <Arduino.h>
#include <ElegantOTA.h>
#include <TaskScheduler.h>
#include <ArduinoLog.h>
#include <wifimanager.h>

#include "configuration.h"

#include "webserver.h"

#include "settings.h"
#include "ambient/ambient.h"
#include "heater.h"
#include "powermonitor.h"
#include <Wire.h>
#include <LittleFS.h>
#include <OneButton.h>
#include "statusled.h"
#include <AsyncTCP.h>
#include <asyncbufferedtcplogger.h>
#include "influxdb.h"
#include <arduinoota-manager.h>
#include <ArduinoOTA.h>

Scheduler scheduler;


AsyncBufferedTCPLogger bufferedLogger{9911, APB_NETWORK_LOGGER_BACKLOG };

#ifdef ONEBUTTON_USER_BUTTON_1
OneButton userButton;
#endif


APB::WebServer webServer(scheduler);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;
using namespace GuLinux;

void setup() {
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
  auto wait_until = millis() + WAIT_FOR_SERIAL; while(!Serial && millis() < wait_until);
  #endif
  #ifdef BOOT_DELAY
  delay(BOOT_DELAY);
  #endif


  bufferedLogger.setup();
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.addHandler(&bufferedLogger);
  Log.infoln(LOG_SCOPE "setup, core: %d", xPortGetCoreID());
  
  

  LittleFS.begin();
  APB::Settings::Instance.setup();
  APB::InfluxDb::Instance.setup(scheduler);
  
  APB::StatusLed::Instance.setup();
  #ifdef WIFI_POWER_TX
  WiFi.setTxPower(WIFI_POWER_TX);
  #endif
  #ifdef WIFI_POWER_RX
  WiFi.setTxPower(WIFI_POWER_RX);
  #endif

  
  WiFiManager::Instance.setOnConnectedCallback(std::bind(&APB::StatusLed::okPattern, &APB::StatusLed::Instance));
  WiFiManager::Instance.setOnConnectionFailedCallback(std::bind(&APB::StatusLed::wifiConnectionFailedPattern, &APB::StatusLed::Instance));
  WiFiManager::Instance.setOnNoStationsFoundCallback(std::bind(&APB::StatusLed::noWiFiStationsFoundPattern, &APB::StatusLed::Instance));
  WiFiManager::Instance.setup(scheduler, &APB::Settings::Instance.wifi());
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  APB::Ambient::Instance.setup(scheduler);
  APB::PowerMonitor::Instance.setup(scheduler);
  std::for_each(APB::Heaters::Instance.begin(), APB::Heaters::Instance.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler); });
  
  webServer.setup();
  ArduinoOTAManager::Instance.setup([](const char*s) { Log.warning(s); }, std::bind(&fs::LittleFSFS::end, &LittleFS));
  APB::History::Instance.setup(scheduler);

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.attachDoubleClick([]() {
    Log.infoln("[OneButton] User button 1 double clicked, reconnecting WiFi");
    WiFiManager::Instance.reconnect();
  });
  userButton.setup(ONEBUTTON_USER_BUTTON_1, INPUT, false);
#endif
}

void loop() {
  WiFiManager::Instance.loop(); 
  scheduler.execute();
  ElegantOTA.loop();

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.tick();
#endif
  ArduinoOTAManager::Instance.loop();
}
