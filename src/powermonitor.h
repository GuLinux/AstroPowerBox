#ifndef APB_POWERMONITOR_H
#define APB_POWERMONITOR_H

#include <optional>
#include <memory>
#include <TaskSchedulerDeclarations.h>

#include "configuration.h"
#include <INA219.h>

namespace APB {

class PowerMonitor {
public:
    static PowerMonitor &Instance;
    PowerMonitor();
    ~PowerMonitor();
    void setup(Scheduler &scheduler);

    struct Status {
        bool initialised = false;
        float shuntVoltage = 0;
        float busVoltage = 0;
        float current = 0;
        float power = 0;
        float charge = 0;
    };
    enum PowerSource {
        AC = 0,
        LipoBattery3C = 1,
    };
    Status status() const { return _status; }
private:
    INA219 _ina219{APB_INA1219_ADDRESS};
    PowerMonitor::Status _status;
    Task _loopTask;
    PowerSource _powerSource = AC;

    void setCharge();
};
}

#endif