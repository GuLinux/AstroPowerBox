#include "statusled.h"

APB::StatusLed::StatusLed(Settings &settings) : settings{settings}, led{APB_STATUS_LED_PIN, APB_STATUS_LED_INVERT_LOGIC} {
}

void APB::StatusLed::setup() {
    led.setup();
    setDuty(settings.statusLedDuty());
    setupPattern();
}

float APB::StatusLed::duty() const {
    return led.duty();
}

void APB::StatusLed::setDuty(float duty) {
    led.setDuty(duty);
    settings.setStatusLedDuty(duty);
}

void APB::StatusLed::setupPattern() {
    led.setPattern(1, 1);
}

void APB::StatusLed::searchingWiFiPattern() {
    led.setPattern(5, 5);
}

void APB::StatusLed::noWiFiStationsFoundPattern() {
    led.setPattern(2, 2, 4, 2);
}

void APB::StatusLed::wifiConnectionFailedPattern() {
    led.setPattern(1, 1, 4, 6);
}

void APB::StatusLed::okPattern() {
    led.setPattern(1, 1, 2, 100, true);
}
