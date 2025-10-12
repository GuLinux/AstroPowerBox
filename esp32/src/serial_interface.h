#pragma once
#include <Print.h>

namespace APB {

class SerialInterface {
public:
    SerialInterface(Print *stream);
private:
    Print *_stream;
};

}