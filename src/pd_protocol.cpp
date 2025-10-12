#include "pd_protocol.h"
#include <ArduinoLog.h>

const std::map<PDProtocol::Voltage, PDProtocol::PinoutConfig> PDProtocol::voltageToCommand = {
    {Voltage::NOT_AVAILABLE, {0, 0, 0}},
    {Voltage::V5, {1, 0, 0}},
    {Voltage::V9, {0, 0, 0}},
    {Voltage::V12, {0, 0, 1}},
    {Voltage::V15, {0, 1, 1}},
};

PDProtocol::PinoutConfig PDProtocol::lastConfig = PDProtocol::voltageToCommand.at(Voltage::NOT_AVAILABLE);

const std::forward_list<PDProtocol::Voltage> PDProtocol::supportedVoltages = {
#if defined(CH224K_CFG1_PIN) && defined(CH224K_CFG2_PIN) && defined(CH224K_CFG3_PIN)
    Voltage::V5,
    Voltage::V9,
    Voltage::V12,
    Voltage::V15,
    #else
    Voltage::NOT_AVAILABLE,
#endif
};

void PDProtocol::setVoltage(Voltage voltage) {
    #if defined(CH224K_CFG1_PIN) && defined(CH224K_CFG2_PIN) && defined(CH224K_CFG3_PIN)
        if(voltage == NOT_AVAILABLE ||  voltageToCommand.find(voltage) == voltageToCommand.end()) {
            Log.error("PDProtocol: Unsupported voltage %dV\n", static_cast<int>(voltage));
            return;
        }
        const auto &[cfg1, cfg2, cfg3] = voltageToCommand.at(voltage);
        pinMode(CH224K_CFG1_PIN, OUTPUT);
        pinMode(CH224K_CFG2_PIN, OUTPUT);
        pinMode(CH224K_CFG3_PIN, OUTPUT);
        digitalWrite(CH224K_CFG1_PIN, cfg1);
        digitalWrite(CH224K_CFG2_PIN, cfg2);
        digitalWrite(CH224K_CFG3_PIN, cfg3);
        lastConfig = std::make_tuple(cfg1, cfg2, cfg3);
        Log.infoln("PDProtocol: Set voltage to %dV (CFG1=%d, CFG2=%d, CFG3=%d)", static_cast<int>(voltage), cfg1, cfg2, cfg3);
    #endif
}

PDProtocol::Voltage PDProtocol::getVoltage() {
    #if defined(CH224K_CFG1_PIN) && defined(CH224K_CFG2_PIN) && defined(CH224K_CFG3_PIN)
        for(const auto &[voltage, pinout] : voltageToCommand) {
            if(pinout == lastConfig) {
                return voltage;
            }
        }
    #endif
    return NOT_AVAILABLE;
}

std::forward_list<PDProtocol::Voltage> PDProtocol::getSupportedVoltages() {
    return supportedVoltages;
}