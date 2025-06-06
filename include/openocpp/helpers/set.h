#ifndef CHARGELAB_OPEN_FIRMWARE_SET_H
#define CHARGELAB_OPEN_FIRMWARE_SET_H

#include <set>

namespace chargelab {
    namespace set {
        template <typename T, typename V>
        bool contains(std::vector<T> const& container, V const& value) {
            return std::find(container.begin(), container.end(), value) != container.end();
        }

        template <typename T>
        bool containsAny(std::vector<T> const&) {
            return false;
        }

        template <typename T, typename V, typename... Tail>
        bool containsAny(std::vector<T> const& container, V const& head, Tail... tail) {
            return contains(container, head) || containsAny(container, std::forward<Tail>(tail)...);
        }
        
        template <typename T, typename V>
        bool contains(std::set<T> const& container, V const& value) {
            return container.find(value) != container.end();
        }

        template <typename T>
        bool containsAny(std::set<T> const&) {
            return false;
        }

        template <typename T, typename V, typename... Tail>
        bool containsAny(std::set<T> const& container, V const& head, Tail... tail) {
            return contains(container, head) || containsAny(container, std::forward<Tail>(tail)...);
        }
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_SET_H
