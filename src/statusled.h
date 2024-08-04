#ifndef APB_STATUS_LED_H
#define APB_STATUS_LED_H

#include "async_led.h"
#include "settings.h"

namespace APB {
class StatusLed {
public:
    StatusLed(Settings &settings);
    void setup();
    float duty() const;
    void setDuty(float duty);

    void setupPattern();
    void searchingWiFiPattern();
    void noWiFiStationsFoundPattern();
    void wifiConnectionFailedPattern();
    void okPattern();
private:
    Settings &settings; 
    AsyncLed led;
};
}
#endif
