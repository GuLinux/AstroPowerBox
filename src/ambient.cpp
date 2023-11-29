#include "ambient.h"

APB::Ambient::Ambient(logging::Logger &logger) : logger{logger} {
}

void APB::Ambient::setup() {
}

float APB::Ambient::temperature() const {
    return 0.0f;
}

float APB::Ambient::humidity() const {
    return 0.0f;
}

float APB::Ambient::dewpoint() const {
    return 0.0f;
}

