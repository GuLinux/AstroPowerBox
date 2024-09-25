#include <WiFi.h>

#include "asyncbufferedtcplogger.h"
#include "configuration.h"

APB::AsyncBufferedTCPLogger::AsyncBufferedTCPLogger(uint16_t port, uint16_t backlog_lines) : loggerServer{port}, backlog_lines{backlog_lines} {
    loggerServer.onClient([this](void *,AsyncClient *c){
      this->client = c;
      c->onDisconnect([this](void *,AsyncClient *){
        this->client = nullptr;
      }, nullptr);
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

void APB::AsyncBufferedTCPLogger::setup() {
  WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info){
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    case ARDUINO_EVENT_WIFI_AP_START:
      this->loggerServer.begin();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    case ARDUINO_EVENT_WIFI_AP_STOP:
      this->loggerServer.end();
      this->client = nullptr;
    default:
      break;
    }
  });
}

size_t APB::AsyncBufferedTCPLogger::write(uint8_t c) {
    buffer[currentPosition++] = c;
    if(c == '\n') {
      if(!client) {
        fillBacklog();
        reset();
        return 0;
      }
      client->write(buffer.data(), currentPosition);
      reset();
    }
    
    return 1;
}

void APB::AsyncBufferedTCPLogger::reset() {
    std::fill(std::begin(buffer), std::end(buffer), 0);
    currentPosition = 0;
}

void APB::AsyncBufferedTCPLogger::fillBacklog() {
  if (backlog_lines > 0) {
    backlog.push(String{buffer.data()});
    while(backlog.size() > backlog_lines) {
      backlog.pop();
    }
  }
}
