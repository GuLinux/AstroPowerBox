#include <Arduino.h>
#include <ElegantOTA.h>
#include <TaskScheduler.h>
#include <ArduinoLog.h>
#include <wifimanager.h>

#include "configuration.h"
#include "commandparser.h"

#include "webserver.h"

#include "settings.h"
#include "ambient/ambient.h"
#include "pwm_output.h"
#include "powermonitor.h"
#include <Wire.h>
#include <LittleFS.h>
#include <OneButton.h>
#include "statusled.h"
#include <AsyncTCP.h>
#include <asyncbufferedtcplogger.h>
#include <arduinoota-manager.h>
#include <ArduinoOTA.h>

Scheduler scheduler;


AsyncBufferedTCPLogger bufferedLogger{9911, APB_NETWORK_LOGGER_BACKLOG };

#ifdef ONEBUTTON_USER_BUTTON_1
OneButton userButton;
#endif


APB::WebServer webServer(scheduler);

Task rescanTask;


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
  
  APB::StatusLed::Instance.setup(&scheduler);
  #ifdef WIFI_POWER_TX
  WiFi.setTxPower(WIFI_POWER_TX);
  #endif
  #ifdef WIFI_POWER_RX
  WiFi.setTxPower(WIFI_POWER_RX);
  #endif

  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t){
    Log.infoln(LOG_SCOPE "WiFi event: %s", WiFi.eventName(event));
  });
  
  rescanTask.set(30'000, TASK_FOREVER, [](){
    Log.infoln(LOG_SCOPE "Rescanning WiFi networks");
    WiFiManager::Instance.rescan();
  });
  scheduler.addTask(rescanTask);
  WiFiManager::Instance.setOnConnectedCallback([](const AsyncWiFiMulti::ApSettings &){
    APB::StatusLed::Instance.okPattern();
    rescanTask.disable();
  });
  WiFiManager::Instance.setOnConnectionFailedCallback([](){
    APB::StatusLed::Instance.wifiConnectionFailedPattern();
    rescanTask.enableDelayed(30'000);
  });
  WiFiManager::Instance.setOnDisconnectedCallback(std::bind(&APB::StatusLed::wifiConnectionFailedPattern, &APB::StatusLed::Instance));
  WiFiManager::Instance.setup(&APB::Settings::Instance.wifi());

  new Task(500, TASK_FOREVER, [](){ WiFiManager::Instance.loop(); }, &scheduler, true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  APB::Ambient::Instance.setup(scheduler);
  APB::PowerMonitor::Instance.setup(scheduler);
  std::for_each(APB::PWMOutputs::Instance.begin(), APB::PWMOutputs::Instance.end(), [i=0](APB::PWMOutput &pwmOutput) mutable { pwmOutput.setup(i++, scheduler); });
  
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

  new Task(100, TASK_FOREVER, [](){ ArduinoOTAManager::Instance.loop(); }, &scheduler, true);
}

void loop() {
  scheduler.execute();

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.tick();
#endif
}
