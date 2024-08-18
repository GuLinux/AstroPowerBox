#pragma once
#include <optional>
#include <functional>
#include <Print.h>
#include <memory>

namespace APB {
class OverflowPrint : public Print {
public:
    OverflowPrint(uint8_t *mainBuffer, size_t mainBufferSize, size_t overflowBufferSize = 512);
    size_t write(uint8_t c);
    size_t setNewBuffer(uint8_t *mainBuffer, size_t mainBufferSize);
    size_t overflow() { return overflowBufferWritten; }
private:
    uint8_t *mainBuffer;
    size_t mainBufferSize;
    size_t mainBufferWritten = 0;
    std::shared_ptr<uint8_t[]> overflowBuffer;
    size_t overflowBufferSize;
    size_t overflowBufferWritten = 0;
};
    namespace optional {
        template<typename T, typename J> void if_present(const std::optional<T> &optional, J f) {
            if(optional.has_value()) {
                f(optional.value());
            }
        }
    }

    String float2s(float f, uint8_t decimals=2);
    class ScopeGuard {
    public:
        ScopeGuard(std::function<void()> onEnd) : onEnd{onEnd} {};
        ScopeGuard(std::function<void()> onStart, std::function<void()> onEnd) : onEnd{onEnd} {
            onStart();
        };
        ~ScopeGuard() { onEnd(); }
    private:
        std::function<void()> onEnd;
    };
}
