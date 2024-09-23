#pragma once
#include <Print.h>
#include <AsyncTCP.h>
#include <queue>

namespace APB {
class AsyncBufferedTCPLogger: public Print {
public:
  AsyncBufferedTCPLogger(AsyncServer &loggerServer);

  virtual size_t write(uint8_t c);
private:
  AsyncClient *client = nullptr;
  void reset();
  size_t fillBacklog();
  std::array<char, 1024> buffer = {0};
  std::queue<String> backlog;
  uint16_t currentPosition = 0;
};

}