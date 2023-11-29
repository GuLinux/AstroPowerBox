#include "heater.h"

namespace {
struct Heater_Private {
    APB::Heater::Mode mode{APB::Heater::Mode::Off};
};
    Heater_Private d;
}

APB::Heater::Heater(logging::Logger &logger) {
}

void APB::Heater::setup() {
}

float APB::Heater::pwm() const {
    return 0.0f;
}

void APB::Heater::pwm(float duty) {
}

bool APB::Heater::temperature(float temperature, float maxDuty) {
    return true;
}

std::optional<float> APB::Heater::temperature() const {
    return std::optional<float>();
}

APB::Heater::Mode APB::Heater::mode() const {
    return d.mode;
}
