#ifndef CHARGELAB_OPEN_FIRMWARE_RING_BUFFER_H
#define CHARGELAB_OPEN_FIRMWARE_RING_BUFFER_H

#include <array>
#include <optional>
#include <cassert>

namespace chargelab {
    template <typename T, int N>
    class RingBuffer {
    public:
        void pushBack(T value) {
            int index;
            if (size_ < (int)buffer_.size()) {
                index = (begin_index_ + size_++) % buffer_.size();
            } else {
                index = begin_index_;
                begin_index_ = (begin_index_ + 1) % buffer_.size();
            }

            buffer_[index] = std::move(value);
        }

        std::optional<T> popFront() {
            if (size_ <= 0)
                return std::nullopt;

            T result = removeAt(begin_index_);
            begin_index_ = (begin_index_ + 1) % buffer_.size();
            size_--;
            return result;
        }

        std::optional<T> popBack() {
            if (size_ <= 0)
                return std::nullopt;

            T result = removeAt((begin_index_ + size_ - 1) % buffer_.size());
            size_--;
            return result;
        }

        T& front() {
            return buffer_[begin_index_];
        }

        T& back() {
            return buffer_[(begin_index_ + size_ - 1) % buffer_.size()];
        }

        T operator[] (int i) const {
            assert(i >= 0 && i < size_);
            return buffer_[(begin_index_ + i) % buffer_.size()];
        }

        T& operator[] (int i) {
            assert(i >= 0 && i < size_);
            return buffer_[(begin_index_ + i) % buffer_.size()];
        }

        [[nodiscard]] bool empty() const {
            return size_ <= 0;
        }

        [[nodiscard]] int size() const {
            return size_;
        }
    private:
        T removeAt(int index) {
            T result {};
            std::swap(result, buffer_[index]);
            return result;
        }
    private:
        std::array<T, N> buffer_;
        int begin_index_ = 0;
        int size_ = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_RING_BUFFER_H
