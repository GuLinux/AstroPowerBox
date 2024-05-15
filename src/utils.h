#ifndef APB_UTILS_H
#define APB_UTILS_H
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
}
#endif