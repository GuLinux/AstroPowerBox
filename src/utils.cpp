#include "utils.h"
#include <cmath>
#include <ArduinoLog.h>

String APB::float2s(float f, uint8_t decimals)
{
    int64_t intPart = static_cast<int64_t>(f);
    int64_t decimalPart = static_cast<int64_t>(pow(10, decimals) * (f - intPart));
    return String(intPart) + "." + String(decimalPart);
}

#define OVERFLOW_TAG "[OVERFLOW] "


APB::OverflowPrint::OverflowPrint(uint8_t *mainBuffer, size_t mainBufferSize, size_t overflowBufferSize) :
    mainBuffer{mainBuffer},
    mainBufferSize{mainBufferSize},
    overflowBuffer{std::make_unique<uint8_t[]>(overflowBufferSize)},
    overflowBufferSize{overflowBufferSize}
{
    Log.traceln(OVERFLOW_TAG "Loaded new overflow object, mainBufferSize=%d, overflowBufferSize=%d", mainBufferSize, overflowBufferSize);
}

size_t APB::OverflowPrint::write(uint8_t c) {
    if(mainBufferWritten < mainBufferSize) {
        mainBuffer[mainBufferWritten++] = c;
        return 1;
    }
    if(overflowBufferWritten < overflowBufferSize) {
        overflowBuffer.get()[overflowBufferWritten++] = c;
        return 0;
    }
    return 0;
}

size_t APB::OverflowPrint::setNewBuffer(uint8_t *mainBuffer, size_t mainBufferSize)
{
    this->mainBuffer = mainBuffer;
    this->mainBufferSize = mainBufferSize;
    Log.traceln(OVERFLOW_TAG "Swapping buffer, mainBufferSize=%d, overflowBufferWritten=%d", mainBufferSize, overflowBufferWritten);
    mainBufferWritten = 0;
    size_t backfill = std::min(mainBufferSize, overflowBufferWritten);
    
    if(backfill > 0) {
        memcpy(mainBuffer, overflowBuffer.get(), backfill);
        mainBufferWritten = backfill;
        overflowBufferWritten -= backfill;
        if(overflowBufferWritten > 0) {
            std::shared_ptr<uint8_t[]> newOverflow = std::make_shared<uint8_t[]>(overflowBufferSize);
            memcpy(newOverflow.get(), &overflowBuffer.get()[backfill], overflowBufferWritten);
            overflowBuffer.swap(newOverflow);
        }
    }
    Log.traceln(OVERFLOW_TAG "Swapping buffer, backfill=%d, overflowBufferWritten=%d", backfill, overflowBufferWritten);
    return backfill;
}

