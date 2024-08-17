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

Scheduler scheduler;
APB::Settings settings;
APB::StatusLed led{settings};
APB::WiFiManager wifiManager{settings, led, scheduler};
APB::Ambient ambient;
APB::Heaters heaters;
APB::PowerMonitor powerMonitor;
APB::History APB::HistoryInstance;
AsyncServer loggerServer{9911};

class BufferedLogger: public Print {
public:
  BufferedLogger() {
    loggerServer.onClient([this](void *,AsyncClient *c){
      this->client = c;
    }, nullptr);
  }

  virtual size_t write(uint8_t c) {
    if(!client) {
      return 0;
    }
    buffer[currentPosition++] = c;
    if(c == '\n') {
      client->write(buffer.data(), currentPosition);
      reset();
    }
    return 1;
  }

private:
  AsyncClient *client = nullptr;
  void reset() {
    std::fill(std::begin(buffer), std::end(buffer), 0);
    currentPosition = 0;
  }
  std::array<char, 1024> buffer = {0};
  uint16_t currentPosition = 0;
};

BufferedLogger bufferedLogger;

#ifdef ONEBUTTON_USER_BUTTON_1
OneButton userButton;
#endif


APB::WebServer webServer(settings, wifiManager, ambient, heaters, powerMonitor, scheduler, led);


#define LOG_SCOPE "APB::Main - "

using namespace std::placeholders;

void setupArduinoOTA();
void addHistoryEntry();

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

  LittleFS.begin();
  settings.setup();
  led.setup();
  
  wifiManager.setup();
  loggerServer.begin();

  Log.addHandler(&bufferedLogger);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  ambient.setup(scheduler);
  powerMonitor.setup(scheduler);
  std::for_each(heaters.begin(), heaters.end(), [i=0](APB::Heater &heater) mutable { heater.setup(i++, scheduler, &ambient); });
  
  webServer.setup();
  setupArduinoOTA();

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.attachDoubleClick([&wifiManager]() {
    Log.infoln("[OneButton] User button 1 double clicked, reconnecting WiFi");
    wifiManager.reconnect();
  });
  userButton.setup(ONEBUTTON_USER_BUTTON_1, INPUT, false);
#endif
  new Task(APB_HISTORY_TASK_SECONDS, TASK_FOREVER, addHistoryEntry, &scheduler, true);
}

uint64_t el = 0;
void loop() {
  wifiManager.loop(); 
  scheduler.execute();
  ElegantOTA.loop();

#ifdef ONEBUTTON_USER_BUTTON_1
  userButton.tick();
#endif
  ArduinoOTA.handle();
}

void addHistoryEntry() {
  APB::History::Entry entry {
    esp_timer_get_time() / 1000'000
  };
  entry.setAmbient(ambient.reading());
  for(uint8_t i=0; i<APB_HEATERS_TEMP_SENSORS; i++) {
    entry.heaters[i].set(heaters[i]);
  }
  entry.setPower(powerMonitor.status());
  APB::HistoryInstance.add(entry);
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
      Serial.println("Start updating " + type);
      LittleFS.end();
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();
}