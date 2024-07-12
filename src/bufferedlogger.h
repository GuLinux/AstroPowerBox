#ifndef BUFFERED_LOGGER_H
#define BUFFERED_LOGGER_H
#include <Print.h>
#include <deque>
#include <stack>

class BufferedLogger : public Print {
public:
    BufferedLogger(uint16_t maxBufferSize, Print *other = nullptr) : maxBufferSize{maxBufferSize}, other{other} {}
    void setLogger(Print *other) {
        this->other = other;

        while(other->availableForWrite() && !buffer.empty()) {
            other->write(buffer.top());
            buffer.pop();
        }
    }

    virtual size_t write(uint8_t c) {
        if(other && other->availableForWrite()) {
            other->write(c);
        } else {
            buffer.push(c);
            while(!buffer.empty() && buffer.size() > maxBufferSize) {
                buffer.pop();
            }
        }
        return 1;
    }

private:
    const uint16_t maxBufferSize;
    Print *other = nullptr;
    std::stack<uint8_t> buffer;
};


#endif
