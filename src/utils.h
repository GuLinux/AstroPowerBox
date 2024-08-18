#pragma once
#include <optional>
#include <functional>

namespace APB {
    namespace optional {
        template<typename T, typename J> void if_present(const std::optional<T> &optional, J f) {
            if(optional.has_value()) {
                f(optional.value());
            }
        }
    }

    static String float2s(float f, uint8_t decimals=2) {
        int64_t intPart = static_cast<int64_t>(f);
        int64_t decimalPart = static_cast<int64_t>(pow(10, decimals) * (f - intPart));
        return String(intPart) + "." + String(decimalPart);
    }

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
