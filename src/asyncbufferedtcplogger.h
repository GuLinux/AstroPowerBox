#pragma once
#include <Print.h>
#include <AsyncTCP.h>

namespace APB {
class AsyncBufferedTCPLogger: public Print {
public:
  AsyncBufferedTCPLogger(AsyncServer &loggerServer);

  virtual size_t write(uint8_t c);
private:
  AsyncClient *client = nullptr;
  void reset();
  std::array<char, 1024> buffer = {0};
  uint16_t currentPosition = 0;
};

}