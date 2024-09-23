#include "asyncbufferedtcplogger.h"
#include "configuration.h"

APB::AsyncBufferedTCPLogger::AsyncBufferedTCPLogger(AsyncServer & loggerServer) {
    loggerServer.onClient([this](void *,AsyncClient *c){
      this->client = c;
      if(!this->backlog.empty()) {
        c->write("==== Flushing backlog ====\n");
        while(!this->backlog.empty()) {
          c->write(this->backlog.front().c_str(), this->backlog.front().length());
          this->backlog.pop();
        }
        c->write("==== Backlog finished ====\n");
      }
    }, nullptr);
}

#include "utils.h"
size_t APB::AsyncBufferedTCPLogger::write(uint8_t c) {
    buffer[currentPosition++] = c;
    if(c == '\n') {
      ScopeGuard resetBuffer(std::bind(&AsyncBufferedTCPLogger::reset, this));
      if(!client) {
        return fillBacklog();
      }
      client->write(buffer.data(), currentPosition);
    }
    return 1;
}

void APB::AsyncBufferedTCPLogger::reset() {
    std::fill(std::begin(buffer), std::end(buffer), 0);
    currentPosition = 0;
}

size_t APB::AsyncBufferedTCPLogger::fillBacklog() {
  #if APB_NETWORK_LOGGER_BACKLOG > 0
    backlog.push(String{buffer.data()});
    while(backlog.size() > APB_NETWORK_LOGGER_BACKLOG) {
      backlog.pop();
    }
  #endif
    return 0;
}
