#ifndef CHARGELAB_OPEN_FIRMWARE_MAP_H
#define CHARGELAB_OPEN_FIRMWARE_MAP_H

#include <utility>

namespace chargelab {
    namespace container {
        template <typename T, typename V>
        bool contains(T&& container, V const& value) {
            return container.find(value) != container.end();
        }

        template <typename T>
        bool containsAny(T&&) {
            return true;
        }

        template <typename T, typename V, typename... Tail>
        bool containsAny(T&& container, V const& head, Tail... tail) {
            return contains(container, head) || containsAny(container, std::forward<Tail>(tail)...);
        }
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_MAP_H
