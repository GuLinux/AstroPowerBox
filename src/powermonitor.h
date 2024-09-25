#ifndef APB_POWERMONITOR_H
#define APB_POWERMONITOR_H

#include <optional>
#include <memory>
#include <TaskSchedulerDeclarations.h>

#include "configuration.h"

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
        
    };
    Status status() const;
private:
    struct Private;
    friend struct Private;
    std::shared_ptr<Private> d;
};
}

#endif