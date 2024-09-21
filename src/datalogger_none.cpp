#include "configuration.h"
#include "datalogger.h"

#if APB_DATALOGGER == APB_DATALOGGER_NONE




APB::DataLogger::DataLogger() {
}

void APB::DataLogger::setup() {
}

bool APB::DataLogger::initialised() const {
    return false;
}

#endif
APB::DataLogger &APB::DataLogger::Instance = *new APB::DataLogger{};
