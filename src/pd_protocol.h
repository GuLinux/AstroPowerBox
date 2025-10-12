#pragma once
#include "configuration.h"
#include <map>
#include <tuple>
#include <forward_list>

class PDProtocol {
public:
    enum Voltage { NOT_AVAILABLE=0, V5 = 5, V9 = 9, V12 = 12, V15 = 15 };

    static void setVoltage(Voltage voltage);
    static Voltage getVoltage();
    static std::forward_list<Voltage> getSupportedVoltages();
private:
    using PinoutConfig = std::tuple<uint8_t, uint8_t, uint8_t>; // (CFG1, CFG2, CFG3)
    static const std::map<Voltage, PinoutConfig> voltageToCommand;
    static const std::forward_list<Voltage> supportedVoltages;
    static PinoutConfig lastConfig;
};