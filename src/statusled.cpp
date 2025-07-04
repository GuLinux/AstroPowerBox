#include "statusled.h"
#include "configuration.h"

APB::StatusLed &APB::StatusLed::Instance = *new APB::StatusLed();

APB::StatusLed::StatusLed() : led{APB_STATUS_LED_PIN, APB_STATUS_LED_INVERT_LOGIC} {
}

void APB::StatusLed::setup(Scheduler *scheduler) {
    led.setup(scheduler);
    setDuty(Settings::Instance.statusLedDuty());
    setupPattern();
}

float APB::StatusLed::duty() const {
    return led.duty();
}

void APB::StatusLed::setDuty(float duty) {
    led.setDuty(duty);
    Settings::Instance.setStatusLedDuty(duty);
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
    led.setPattern(1, 1, 2, 50, true);
}
