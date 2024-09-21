#include "configuration.h"

#if APB_DATALOGGER == APB_DATALOGGER_SD
#include "datalogger.h"

#include <ArduinoLog.h>

#include "FS.h"
#include "SPI.h"
#include <SdFat.h>  
#include <StreamString.h>

#define LOG_SCOPE "DataLogger::SD - "

namespace {
    bool dataLogger_initialised = false;
    SdFat32 SD;

    #ifdef APB_SPI_MISO
    SPIClass spi(HSPI);
    SdSpiConfig spiConfig(APB_SD_CS, DEDICATED_SPI, SPI_QUARTER_SPEED, &spi);
    #endif
}

APB::DataLogger::DataLogger() {
}

void APB::DataLogger::setup() {
    pinMode(APB_SD_CS, OUTPUT);
    digitalWrite(APB_SD_CS, LOW);
    #ifdef APB_SPI_MISO
    spi.begin(APB_SPI_CLK, APB_SPI_MISO, APB_SPI_MOSI);
    dataLogger_initialised = SD.begin(spiConfig);
    #else
    dataLogger_initialised = SD.begin(APB_SD_CS, SPI_HALF_SPEED);
    #endif
    if(!dataLogger_initialised) {
        StreamString errorDetails;
        SD.initErrorPrint(&errorDetails);
        Log.errorln(LOG_SCOPE "Unable to initialise SD Card: %s", errorDetails.c_str());
        
    } else {
        Log.infoln(LOG_SCOPE "SD Initialised correctly. Card size: %d", SD.card()->sectorCount());
    }
}

bool APB::DataLogger::initialised() const {
    return dataLogger_initialised;
}

#endif