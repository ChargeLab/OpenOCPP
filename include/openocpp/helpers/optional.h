#ifndef CHARGELAB_OPEN_FIRMWARE_OPTIONAL_H
#define CHARGELAB_OPEN_FIRMWARE_OPTIONAL_H

#include <optional>

namespace chargelab::optional {
    template <typename T, typename C>
    inline void IfPresent(std::optional<T> const& container, C&& consumer) {
        if (container.has_value()) {
            consumer(container.value());
        }
    }

    template <typename T>
    inline T GetOrDefault(std::optional<T> const& currentValue, T const& defaultValue) {
        if (currentValue.has_value()) {
            return currentValue.value();
        } else {
            return defaultValue;
        }
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_OPTIONAL_H
