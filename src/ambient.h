#ifndef APB_AMBIENT_H
#define APB_AMBIENT_H

#include <TaskSchedulerDeclarations.h>
#include <optional>

#include "configuration.h"


namespace APB {

class Ambient {
public:
    Ambient();
    void setup(Scheduler &scheduler);
    struct Reading {
        float temperature;
        float humidity;
        float dewpoint() const;
    };
    std::optional<Reading> reading() const;
    #ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    void setSim(float temperature, float humidity, bool initialised=true);
    #endif
    struct Private;
};

}

#endif