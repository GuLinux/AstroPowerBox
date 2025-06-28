#include "history.h"
#include <ArduinoLog.h>
#include "utils.h"
#include <iterator>
#include "configuration.h"
#include <memory>

APB::History &APB::History::Instance = *new APB::History{};


APB::History::History() {
}


#if APB_PWM_OUTPUTS_SIZE > 0
void APB::History::Entry::PWMOutput::set(const APB::PWMOutput &pwmOutput) {
    temperatureHundredth = static_cast<int16_t>(pwmOutput.temperature().value_or(-100.0) * 100.0);
    duty = pwmOutput.active() ? pwmOutput.duty() : 0;
}
#endif

#ifndef APB_AMBIENT_TEMPERATURE_SENSOR_NONE
void APB::History::Entry::setAmbient(const std::optional<Ambient::Reading> &reading) {
    const auto readingValue = reading.value_or(Ambient::Reading{-100.0, -100.0});
    ambientTemperatureHundredth = static_cast<uint16_t>(readingValue.temperature * 100.0);
    ambientHumidityHundredth = static_cast<uint16_t>(readingValue.humidity * 100.0);
}
#endif

void APB::History::Entry::setPower(const PowerMonitor::Status & powerStatus) {
    busVoltageHundreth = static_cast<uint16_t>(powerStatus.busVoltage * 100.0);
    currentHundreth = static_cast<uint16_t>(powerStatus.current * 100.0);
}

void APB::History::Entry::populate(JsonObject object) {
    object["uptime"] = secondsFromBoot;

#ifndef APB_AMBIENT_TEMPERATURE_SENSOR_NONE
    setNullableFloat(object, "ambientTemperature", getAmbientTemperature());
    setNullableFloat(object, "ambientHumidity", getAmbientHumidity());
    object["ambientDewpoint"] = getDewpoint();
#else
    object["ambientTemperature"] = static_cast<char*>(0);
    object["ambientHumidity"] = static_cast<char*>(0);
    object["ambientDewpoint"] = static_cast<char*>(0);
#endif

#if APB_PWM_OUTPUTS_SIZE > 0
    for(uint8_t i=0; i<pwmOutputs.size(); i++) {
        JsonObject pwmOutput = object["pwmOutputs"][i].to<JsonObject>();;
        pwmOutput["duty"] = pwmOutputs[i].getDuty();
        setNullableFloat(pwmOutput, "temperature", pwmOutputs[i].getTemperature());
    }
#else
    object["pwmOutputs"].to<JsonArray>();
#endif
    object["busVoltage"] = getBusVoltage();
    object["power"] = getPower();
    object["current"] = getCurrent();
}


void APB::History::Entry::setNullableFloat(JsonObject object, const char *field, float value, float minValue) {
    if(value < minValue) {
        object[field] = (char*)0;
    } else {
        object[field] = value;
    }
}

void APB::History::add() {
  if(lockInserts) {
      Log.warningln("[HISTORY] Inserts locked, unable to add entry");
      return;
  }

  APB::History::Entry entry {
    static_cast<uint32_t>(esp_timer_get_time() / 1000'000)
  };

#ifndef APB_AMBIENT_TEMPERATURE_SENSOR_NONE
  entry.setAmbient(APB::Ambient::Instance.reading());
#endif
  
#if APB_PWM_OUTPUTS_SIZE > 0
  for(uint8_t i=0; i<APB_PWM_OUTPUTS_TEMP_SENSORS; i++) {
    entry.pwmOutputs[i].set(APB::PWMOutputs::Instance[i]);
  }
#endif
  entry.setPower(PowerMonitor::Instance.status());

  _entries.push_back(entry);
  while(_entries.size() > maxSize) {
      _entries.pop_front();
  }
}


#define JSON_SERIALISER_TAG "[History::JsonSerialiser] "

APB::History::JsonSerialiser::JsonSerialiser(History &history) : history{history} {
}

int APB::History::JsonSerialiser::write(uint8_t *buffer, size_t maxLen, size_t index) {
    // Log.traceln(JSON_SERIALISER_TAG "===> write: bufferMaxLen=%d, index=%d", maxLen, index);
    int response = 0;
    if(index == 0) {
        overflowPrint = std::make_unique<OverflowPrint>(buffer, maxLen);
    } else {
        response += overflowPrint->setNewBuffer(buffer, maxLen);
    }
    
    
    // ScopeGuard printResponseSize = {[&](){
    //     Log.traceln(JSON_SERIALISER_TAG "<=== exiting write, response=%d, overflow=%d", response, overflowPrint->overflow());
    // }};
    
    if(!headerCreated) {
        response = overflowPrint->printf("{\"now\":%d,\"entries\":[", esp_timer_get_time() / 1'000'000);
        // Log.traceln(JSON_SERIALISER_TAG "Creating header, response=%d", response);
        headerCreated = true;
        it = history._entries.begin();
    }
    if(history._entries.empty()) {
        response = overflowPrint->printf("]}");
        footerCreated = true;
    }
    if(footerCreated) {
        // Log.traceln(JSON_SERIALISER_TAG "Footer already sent, exiting");
        return response;
    }
    while(overflowPrint->overflow() == 0 && response < maxLen && it != history._entries.end()) {
        // Log.traceln(JSON_SERIALISER_TAG "Sending item at index=%d of %d", currentIndex, history._entries.size());
        it->populate(jsonDocument.to<JsonObject>());
        response += serializeJson(jsonDocument, *overflowPrint);
        std::advance(it, 1);
        // Log.traceln(JSON_SERIALISER_TAG "Item[%d] sent, is last: %d", currentIndex, it == history._entries.end());
        currentIndex++;
        if(it != history._entries.end()) {
            response += overflowPrint->print(',');
        } else {
            response += overflowPrint->print("]}");
            footerCreated = true;
        }
    }
    return response;
}

void APB::History::setup(Scheduler &scheduler) {
  new Task(APB_HISTORY_TASK_SECONDS, TASK_FOREVER, std::bind(&History::add, this), &scheduler, true);
}
