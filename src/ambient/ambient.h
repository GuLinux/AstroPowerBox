#ifndef APB_AMBIENT_H
#define APB_AMBIENT_H

#include <TaskSchedulerDeclarations.h>
#include <optional>

#include "configuration.h"


namespace APB {

class Ambient {
public:
    static Ambient &Instance;
    Ambient();
    void setup(Scheduler &scheduler);
    struct Reading {
        float temperature;
        float humidity;
        float dewpoint() const;
    };
    std::optional<Reading> reading() const { return _reading; };
    bool isInitialised() const;

    Task readValuesTask;
    bool initialised = false;
    bool initialiseSensor();
    void readSensor();
    std::optional<Reading> _reading;
    static float calculateDewpoint(float temperature, float humidity);
};

}

#endif