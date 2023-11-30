#ifndef APB_AMBIENT_H
#define APB_AMBIENT_H

#include <logger.h>
#include "configuration.h"
#include <TaskSchedulerDeclarations.h>

namespace APB {

class Ambient {
public:
    Ambient(logging::Logger &logger);
    void setup(Scheduler &scheduler);
    float temperature() const;
    float humidity() const;
    float dewpoint() const;
    #ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    void setSim(float temperature, float humidity);
    #endif
private:
    logging::Logger &logger;
    void readValues();
    Task readValuesTask;
};

}

#endif