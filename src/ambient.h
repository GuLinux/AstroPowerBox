#ifndef APB_AMBIENT_H
#define APB_AMBIENT_H


#include <TaskSchedulerDeclarations.h>

#include "configuration.h"


namespace APB {

class Ambient {
public:
    Ambient();
    void setup(Scheduler &scheduler);
    float temperature() const;
    float humidity() const;
    float dewpoint() const;
    #ifdef APB_AMBIENT_TEMPERATURE_SENSOR_SIM
    void setSim(float temperature, float humidity);
    #endif
private:
    void readValues();
};

}

#endif