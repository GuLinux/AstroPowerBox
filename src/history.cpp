#include "history.h"
#include <ArduinoLog.h>
#include "utils.h"
#include <iterator>

void APB::History::Entry::Heater::set(const APB::Heater &heater) {
    temperatureHundredth = static_cast<int16_t>(heater.temperature().value_or(-100.0) * 100.0);
    duty = heater.active() ? heater.duty() : 0;
}

void APB::History::Entry::setAmbient(const std::optional<Ambient::Reading> &reading) {
    const auto readingValue = reading.value_or(Ambient::Reading{-100.0, -100.0});
    ambientTemperatureHundredth = static_cast<uint16_t>(readingValue.temperature * 100.0);
    ambientHumidityHundredth = static_cast<uint16_t>(readingValue.humidity * 100.0);
}

void APB::History::Entry::setPower(const PowerMonitor::Status & powerStatus) {
    busVoltageHundreth = static_cast<uint16_t>(powerStatus.busVoltage * 100.0);
    currentHundreth = static_cast<uint16_t>(powerStatus.current * 100.0);
}

void APB::History::Entry::populate(JsonObject object) {
    object["uptime"] = secondsFromBoot;
    setNullableFloat(object, "ambientTemperature", getAmbientTemperature());
    setNullableFloat(object, "ambientHumidity", getAmbientHumidity());
    object["ambientDewpoint"] = getDewpoint();
    for(uint8_t i=0; i<heaters.size(); i++) {
        JsonObject heaterObject = object["heaters"][i].to<JsonObject>();;
        heaterObject["duty"] = heaters[i].getDuty();
        setNullableFloat(heaterObject, "temperature", heaters[i].getTemperature());
    }
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



void APB::History::add(const Entry &entry) {
    if(lockInserts) {
        Log.warningln("[HISTORY] Inserts locked, unable to add entry");
        return;
    }
    _entries.push_back(entry);
    while(_entries.size() > maxSize) {
        _entries.pop_front();
    }
}

#define JSON_SERIALISER_TAG "[History::JsonSerialiser] "

APB::History::JsonSerialiser::JsonSerialiser(History &history) : history{history} {
    // Log.traceln(JSON_SERIALISER_TAG "created");
    // history.lockInserts = true;
    jsonBufferSize = sprintf(jsonBuffer, "{\"now\":%d,\"entries\":[", esp_timer_get_time() / 1'000'000);
}

int APB::History::JsonSerialiser::write(uint8_t *buffer, size_t maxLen, size_t index) {
    // Log.traceln(JSON_SERIALISER_TAG "chunkedResponseCb, maxLen=%d, index=%d", maxLen, index);
    if(currentBufferIndex >= jsonBufferSize) {
        // Log.traceln(JSON_SERIALISER_TAG "currentBufferIndex (%d)> jsonBufferSize (%d)", currentBufferIndex, jsonBufferSize);
        currentBufferIndex = 0;
        if(!firstEntrySent) {
            // Log.traceln(JSON_SERIALISER_TAG "preparing buffer to first entry");
            it = history._entries.begin();
            populateEntry();
            firstEntrySent = true;
        } else {
            if(footerCreated) {
                // Log.traceln(JSON_SERIALISER_TAG "buffer all sent, returning 0");
                return 0;
            }
            if(it == history._entries.end()) {
                // Log.traceln(JSON_SERIALISER_TAG "entries all send, preparing final chunk");
                jsonBufferSize = sprintf(jsonBuffer, "]}");
                footerCreated = true;
            } else {
                // Log.traceln(JSON_SERIALISER_TAG "populating next entry");
                populateEntry();
            }
        }
    }
    int bytesToSend = std::min(maxLen, jsonBufferSize - currentBufferIndex);
    // Log.traceln(JSON_SERIALISER_TAG "sending data: bytesToSend=%d, jsonBufferSize=%d, currentBufferIndex=%d", bytesToSend, jsonBufferSize, currentBufferIndex);
    memcpy(buffer, &jsonBuffer[currentBufferIndex], bytesToSend);
    currentBufferIndex += bytesToSend;
    // Log.traceln(JSON_SERIALISER_TAG "sent %d bytes", bytesToSend);
    return bytesToSend;
}

void APB::History::JsonSerialiser::populateEntry() {
    // Log.traceln(JSON_SERIALISER_TAG "populateEntry: currentIndex=%d, maxElements=%d, isLast=%d, isPastLast=%d",
    //     currentIndex,
    //     history._entries.size(),
    //     std::next(it) == history._entries.end(),
    //     it == history._entries.end()
    // );
    it->populate(jsonDocument.to<JsonObject>());
    jsonBufferSize = serializeJson(jsonDocument, jsonBuffer);
    std::advance(it, 1);
    currentIndex++;
    if(it != history._entries.end()) {
        // Log.traceln(JSON_SERIALISER_TAG "not at the end of list, adding comma");
        jsonBuffer[jsonBufferSize] = ',';
        jsonBufferSize += 1;
    }
}

