#include "asyncbufferedtcplogger.h"

APB::AsyncBufferedTCPLogger::AsyncBufferedTCPLogger(AsyncServer & loggerServer) {
    loggerServer.onClient([this](void *,AsyncClient *c){
      this->client = c;
    }, nullptr);
}

size_t APB::AsyncBufferedTCPLogger::write(uint8_t c) {
    if(!client) {
      return 0;
    }
    buffer[currentPosition++] = c;
    if(c == '\n') {
      client->write(buffer.data(), currentPosition);
      reset();
    }
    return 1;
}

void APB::AsyncBufferedTCPLogger::reset() {
    std::fill(std::begin(buffer), std::end(buffer), 0);
    currentPosition = 0;
}
