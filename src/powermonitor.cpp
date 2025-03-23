#include "powermonitor.h"
#include <ArduinoLog.h>
#include "configuration.h"
#include <array>
#include <utility>
#include "settings.h"


float lipoBatteryCharge(uint8_t cells, float voltage);

APB::PowerMonitor &APB::PowerMonitor::Instance = *new APB::PowerMonitor();

APB::PowerMonitor::PowerMonitor() {
}

APB::PowerMonitor::~PowerMonitor() {
}

void APB::PowerMonitor::setup(Scheduler &scheduler) {
    _status.initialised = _ina219.begin();
    if(_status.initialised) {
        Log.infoln("Powermonitor initialised: INA219 with address 0x%x", APB_INA1219_ADDRESS);
        
        
        bool gainValid = _ina219.setGain(APB_POWER_INA219_GAIN);
        bool voltageRangeValid = _ina219.setBusVoltageRange(APB_POWER_INA219_VOLTAGE_RANGE);
        bool shuntValid = _ina219.setMaxCurrentShunt(APB_POWER_MAX_CURRENT_AMPS, APB_POWER_SHUNT_OHMS);
    
        Log.infoln("Powermonitor settings: valid=%d, %d milliamp max (%d amps), shunt resistor: %d milliohms",
            shuntValid && voltageRangeValid && gainValid,
            static_cast<int>(_ina219.getMaxCurrent() * 1000.0),
            static_cast<int>(_ina219.getMaxCurrent()),
            static_cast<int>(_ina219.getShunt() * 1000.0)
        );

        _loopTask.set(1000, TASK_FOREVER, [this](){
            _status.busVoltage = _ina219.getBusVoltage();
            _status.current = _ina219.getCurrent();
            _status.power = _ina219.getPower();
            _status.shuntVoltage = _ina219.getShuntVoltage();
            setCharge();
            if(_status.power == 0 && _status.current == 0) {
                Log.warningln("Powermonitor: Reporting power as 0. INA status: %d", _ina219.isConnected());
            }
        });
        scheduler.addTask(_loopTask);
        _loopTask.enable();
    } else {
        Log.errorln("Powermonitor failed to initialise INA219 with address 0x%x", APB_INA1219_ADDRESS);
    }
}

void APB::PowerMonitor::toJson(JsonObject powerStatus) {
    powerStatus["busVoltage"] = _status.busVoltage;
    powerStatus["current"] = _status.current;
    powerStatus["power"] = _status.power;
    powerStatus["shuntVoltage"] = _status.shuntVoltage;
    powerStatus["charge"] = _status.charge;
}

void APB::PowerMonitor::setCharge()
{
    switch (Settings::Instance.powerSource()) {
    case LipoBattery3C:
        _status.charge = lipoBatteryCharge(3, _status.busVoltage);
        break;
    default:
        _status.charge = 100.0f;
    }
}

float lipoBatteryCharge(uint8_t cells, float voltage) {
    float singleCellVoltage = voltage / static_cast<float>(cells);
    using RefPair = std::pair<float, float>;
    static const std::array<RefPair, 20> chargeMap{{
        {95, 4.15},
        {90, 4.11},
        {85, 4.08},
        {80, 4.02},
        {75, 3.98},
        {70, 3.95},
        {65, 3.91},
        {60, 3.87},
        {55, 3.85},
        {50, 3.84},
        {45, 3.82},
        {40, 3.8},
        {35, 3.79},
        {30, 3.77},
        {25, 3.75},
        {20, 3.73},
        {15, 3.71},
        {10, 3.69},
        { 5, 3.61},
        { 0, 3.27},
    }};
    const RefPair *found = std::find_if(chargeMap.begin(), chargeMap.end(), [&singleCellVoltage](const RefPair &el){ return singleCellVoltage > el.second; });
    if(found == chargeMap.end()) {
        return 100.0;
    }
    return found->first;
}