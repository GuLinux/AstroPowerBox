#include "influxdb.h"
#include "configuration.h"

APB::InfluxDb &APB::InfluxDb::Instance = *new APB::InfluxDb{};

#ifdef USE_INFLUXDB
#include "settings.h"
#include "wifimanager.h"
#include "powermonitor.h"
#include "heater.h"
#include "ambient/ambient.h"
#include <StreamString.h>
#include <ArduinoLog.h>


#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#define TZ_INFO "UTC1"

void setPointSource(Point &point) {
    point.addTag("source", APB::Settings::Instance.apConfiguration().essid);
}

InfluxDBClient influxDbClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point powerSensor("power");
Point ambientSensor("ambient");
Point heatersSensor("heaters");

void APB::InfluxDb::setup(Scheduler & scheduler) {
    setPointSource(powerSensor);
    setPointSource(ambientSensor);
    setPointSource(heatersSensor);
    WiFiManager::Instance.addOnConnectedListener(std::bind(&InfluxDb::onWiFiConnected, this));

    new Task(APB_INFLUXDB_TASK_SECONDS, TASK_FOREVER, std::bind(&InfluxDb::sendData, this), &scheduler, true);
}

void APB::InfluxDb::onWiFiConnected() {
    Log.infoln("InfluxDB: onWiFiConnected");
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    connected = influxDbClient.validateConnection();
    if(connected) {
        Log.infoln("InfluxDB Connected");
    } else {
        Log.errorln("InfluxDB connection error: %s", influxDbClient.getLastErrorMessage().c_str());
    }
}


void sendPoint(Point &point) {
    bool written = influxDbClient.writePoint(point);
    Log.traceln("InfluxDB: Sending point <%s>: %d", powerSensor.toLineProtocol().c_str(), written);
    if(!written) {
      Log.errorln("InfluxDB last error: %s", influxDbClient.getLastErrorMessage().c_str());
    }
}

void APB::InfluxDb::sendData() {
    if(!connected) {
        return;
    }
    powerSensor.clearFields();
    auto powerReading = PowerMonitor::Instance.status();
    powerSensor.addField("voltage", powerReading.busVoltage);
    powerSensor.addField("current", powerReading.current);
    powerSensor.addField("power", powerReading.power);
    sendPoint(powerSensor);
    
#ifndef APB_AMBIENT_TEMPERATURE_SENSOR_NONE
    auto ambientReading = Ambient::Instance.reading();
    if(ambientReading.has_value()) {
        ambientSensor.addField("temperature", ambientReading->temperature);
        ambientSensor.addField("humidity", ambientReading->humidity);
        ambientSensor.addField("dewpoint", ambientReading->dewpoint());
        sendPoint(ambientSensor);
    }
#endif
#if APB_HEATERS_SIZE > 0
    for(Heater &heater : Heaters::Instance) {
        heatersSensor.clearFields();
        heatersSensor.addField("index", heater.index());
        if(heater.temperature().has_value()) {
            heatersSensor.addField("temperature", heater.temperature().value());
        }
        if(heater.targetTemperature().has_value()) {
            heatersSensor.addField("target_temperature", heater.targetTemperature().value());
        }
        heatersSensor.addField("duty", heater.duty());
        heatersSensor.addField("mode", heater.mode());
        heatersSensor.addField("active", heater.active());
        sendPoint(heatersSensor);
    }
#endif
}
#else
void APB::InfluxDb::setup(Scheduler & scheduler) {}
void APB::InfluxDb::onWiFiConnected() {}
void APB::InfluxDb::sendData() {}
#endif