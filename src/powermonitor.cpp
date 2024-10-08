#include "powermonitor.h"
#include <ArduinoLog.h>
#include "configuration.h"
#include <INA219.h>

APB::PowerMonitor &APB::PowerMonitor::Instance = *new APB::PowerMonitor();

struct APB::PowerMonitor::Private {
    INA219 ina219{APB_INA1219_ADDRESS};
    PowerMonitor::Status status;
    Task loopTask;
};

APB::PowerMonitor::PowerMonitor() : d{std::make_shared<Private>()}{

}

APB::PowerMonitor::~PowerMonitor() {

}

APB::PowerMonitor::Status APB::PowerMonitor::status() const {
    return d->status;
}

void APB::PowerMonitor::setup(Scheduler &scheduler) {
    d->status.initialised = d->ina219.begin();
    if(d->status.initialised) {
        Log.infoln("Powermonitor initialised: INA219 with address 0x%x", APB_INA1219_ADDRESS);
        
        
        bool gainValid = d->ina219.setGain(APB_POWER_INA219_GAIN);
        bool voltageRangeValid = d->ina219.setBusVoltageRange(APB_POWER_INA219_VOLTAGE_RANGE);
        bool shuntValid = d->ina219.setMaxCurrentShunt(APB_POWER_MAX_CURRENT_AMPS, APB_POWER_SHUNT_OHMS);
    
        Log.infoln("Powermonitor settings: valid=%d, %d milliamp max (%d amps), shunt resistor: %d milliohms",
            shuntValid && voltageRangeValid && gainValid,
            static_cast<int>(d->ina219.getMaxCurrent() * 1000.0),
            static_cast<int>(d->ina219.getMaxCurrent()),
            static_cast<int>(d->ina219.getShunt() * 1000.0)
        );

        d->loopTask.set(1000, TASK_FOREVER, [this](){
            d->status.busVoltage = d->ina219.getBusVoltage();
            d->status.current = d->ina219.getCurrent();
            d->status.power = d->ina219.getPower();
            d->status.shuntVoltage = d->ina219.getShuntVoltage();
            if(d->status.power == 0 && d->status.current == 0) {
                Log.warningln("Powermonitor: Reporting power as 0. INA status: %d", d->ina219.isConnected());
            }
        });
        scheduler.addTask(d->loopTask);
        d->loopTask.enable();
    } else {
        Log.errorln("Powermonitor failed to initialise INA219 with address 0x%x", APB_INA1219_ADDRESS);
    }
}