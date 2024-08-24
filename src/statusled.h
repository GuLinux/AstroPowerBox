#ifndef APB_STATUS_LED_H
#define APB_STATUS_LED_H

#include "async_led.h"
#include "settings.h"

namespace APB {
class StatusLed {
public:
    static StatusLed &Instance;
    StatusLed();
    void setup();
    float duty() const;
    void setDuty(float duty);

    void setupPattern();
    void searchingWiFiPattern();
    void noWiFiStationsFoundPattern();
    void wifiConnectionFailedPattern();
    void okPattern();
private:
    AsyncLed led;
};
}
#endif
