#include "powermonitor.h"
#include <ArduinoLog.h>
#include "configuration.h"
#include <INA219.h>

struct APB::PowerMonitor::Private {
    INA219 ina219{0x40};
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
        d->ina219.setMaxCurrentShunt(15, 0.04);
    
        d->loopTask.set(1000, TASK_FOREVER, [this](){
            d->status.busVoltage = d->ina219.getBusVoltage();
            d->status.current = d->ina219.getCurrent();
            d->status.power = d->ina219.getPower();
            d->status.shuntVoltage = d->ina219.getShuntVoltage();
        });
        scheduler.addTask(d->loopTask);
        d->loopTask.enable();
    }
}