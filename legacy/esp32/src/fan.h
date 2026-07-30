#pragma once
#include <esp32-hal-ledc.h>

class Fan {
private:
    Fan();
    float _duty;
    uint8_t channel;
public:
    static Fan &Instance();
    void setup(uint32_t frequency=20'000, float duty=0.f);
    void setDuty(float duty);
    float duty() const;
    ~Fan();
};

