#include "fan.h"
#include "configuration.h"
#include <ArduinoLog.h>

#define LOG_SCOPE "APB::Fan "

Fan::Fan() : _duty{0.0f}, channel{7} {
}

Fan &Fan::Instance()
{
    static Fan instance;
    return instance;
}

void Fan::setup(uint32_t frequency, float duty)
{
    pinMode(APB_PWM_FAN_PIN, OUTPUT);
    for(channel=0; channel < 16; ++channel) {
        if(ledcSetup(channel, frequency, 8) != 0) {
            Log.infoln(LOG_SCOPE "ledcSetup channel %d succeeded, attaching pin", channel);
            break;
        } else {
            Log.warning(LOG_SCOPE "ledcSetup channel %d failed, trying next channel", channel);
        }
    }
    ledcAttachPin(APB_PWM_FAN_PIN, channel); 
    setDuty(duty);
}

void Fan::setDuty(float duty) {
    _duty = duty;
    int analogValue = static_cast<int>(duty * 255.0);
    Log.infoln(LOG_SCOPE "Setting fan duty to %F (analog value %d) to channel %d", duty, analogValue, channel);
    ledcWrite(channel, analogValue);
}
float Fan::duty() const {
    return _duty;
}

Fan::~Fan() {
    ledcWrite(channel, 0);
}