#ifndef CHARGELAB_OPEN_FIRMWARE_SMALL_STRING_H
#define CHARGELAB_OPEN_FIRMWARE_SMALL_STRING_H

#include "openocpp/common/logging.h"

#include <string>
#include <exception>

namespace chargelab {
    class SmallString {
    public:
        ~SmallString() {
            if (allocated_) {
                delete [] str_;
                str_ = nullptr;
                allocated_ = false;
            }
        }

        SmallString() {
            str_ = nullptr;
            size_ = 0;
            allocated_ = false;
        }

        // TODO: This can be improved with various overrides, wrappers, and constexpr to restrict this constructor to
        //       string literals more reliably, but this is likely sufficient for now.
        template <std::size_t N>
        SmallString(char const (&literal)[N]) {
            str_ = literal;
            size_ = N-1;
            allocated_ = false;
        }

        SmallString(std::string const& text) {
            size_ = text.size();
            allocated_ = true;

            auto block = new char[size_];
            std::memcpy(block, text.data(), size_);
            str_ = block;
        }

        SmallString(SmallString const& other) {
            if (other.allocated_) {
                size_ = other.size_;
                allocated_ = true;

                auto block = new char[size_];
                std::memcpy(block, other.str_, other.size_);
                str_ = block;
            } else {
                str_ = other.str_;
                size_ = other.size_;
                allocated_ = other.allocated_;
            }
        }

        SmallString(SmallString&& other) noexcept {
            std::swap(str_, other.str_);
            std::swap(size_, other.size_);
            std::swap(allocated_, other.allocated_);
        }

        SmallString& operator=(SmallString const& other) {
            if (this == &other)
                return *this;

            if (allocated_) {
                delete str_;
                str_ = nullptr;
                allocated_ = false;
            }

            if (other.allocated_) {
                size_ = other.size_;
                allocated_ = true;

                auto block = new char[size_];
                std::memcpy(block, other.str_, other.size_);
                str_ = block;
            } else {
                str_ = other.str_;
                size_ = other.size_;
                allocated_ = other.allocated_;
            }

            return *this;
        }

        SmallString& operator=(SmallString&& other) noexcept {
            if (this == &other)
                return *this;

            std::swap(str_, other.str_);
            std::swap(size_, other.size_);
            std::swap(allocated_, other.allocated_);
            return *this;
        }

        [[nodiscard]] std::string value() const {
            if (str_ != nullptr) {
                return {str_, size_};
            } else {
                return {};
            }
        }

        bool operator==(SmallString const& rhs) const {
            return value() == rhs.value();
        }

        bool operator!=(SmallString const& rhs) const {
            return !operator==(rhs);
        }

    private:
        char const* str_ = nullptr;
        std::size_t size_ = 0;
        bool allocated_ = false;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_SMALL_STRING_H
