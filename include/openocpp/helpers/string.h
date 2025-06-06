#ifndef CHARGELAB_OPEN_FIRMWARE_STRING_H
#define CHARGELAB_OPEN_FIRMWARE_STRING_H

#include <string>
#include <array>
#include <optional>
#include <limits.h>
#include <cstdint>

namespace chargelab::string {
    namespace detail {
        char const kHexCharacters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D' ,'E', 'F'};
    }

    inline bool BeginsWithIgnoreCaseAscii(std::string const& text, std::string const& prefix) {
        if (text.length() < prefix.length())
            return false;

        auto text_it = text.begin();
        auto prefix_it = prefix.begin();
        while (prefix_it != prefix.end()) {
            auto text_ch = *(text_it++);
            auto prefix_ch = *(prefix_it++);
            if (std::tolower(text_ch) != std::tolower(prefix_ch)) {
                return false;
            }
        }

        return true;
    }

    inline bool EqualsIgnoreCaseAscii(std::string const& lhs, std::string const& rhs) {
        return lhs.size() == rhs.size() && BeginsWithIgnoreCaseAscii(lhs, rhs);
    }

    inline std::optional<int> ToInteger(std::string const& text) {
        if (text.empty())
            return std::nullopt;

        errno = 0;
        char* end = nullptr;
        auto const result = strtol(text.c_str(), &end, 10);
        auto const parse_error = errno;
        if (end != text.c_str() + text.size()) {
            return std::nullopt;
        }
        if (parse_error != 0) {
            return std::nullopt;
        }

        return result;
    }

    inline std::optional<std::int64_t> ToInteger64(std::string const& text) {
        if (text.empty())
            return std::nullopt;

        errno = 0;
        char* end = nullptr;
        auto const result = strtoll(text.c_str(), &end, 10);
        auto const parse_error = errno;
        if (end != text.c_str() + text.size()) {
            return std::nullopt;
        }
        if (parse_error != 0) {
            return std::nullopt;
        }

        return result;
    }

    inline std::optional<double> ToDouble(std::string const& text) {
        if (text.empty())
            return std::nullopt;

        errno = 0;
        char* end = nullptr;
        auto const result = strtod(text.c_str(), &end);
        auto const parse_error = errno;
        if (end != text.c_str() + text.size()) {
            return std::nullopt;
        }
        if (parse_error != 0) {
            return std::nullopt;
        }

        return result;
    }

    template<typename T>
    typename std::enable_if<
            std::is_integral<T>::value,
            std::string
    >::type ToHexValue(T value) {
        if (value == 0)
            return "0x0";

        std::string result;
        if (value < 0) {
            value = -value;
            result = "-0x";
        } else {
            result = "0x";
        }

        for (int i=0; i < (int)sizeof(T); i++) {
            auto const offset = (sizeof(T) - i - 1) * CHAR_BIT;
            auto const byte = (value >> offset) & 0xFF;
            result += detail::kHexCharacters[byte >> 4];
            result += detail::kHexCharacters[byte & 0xF];
        }

        return result;
    }

    inline std::string ToHexString(std::uint8_t const* begin, std::uint8_t const* end, char const* separator = " ") {
        std::string result;
        char const* ifs = "";
        for (auto it = begin; it != end; it++) {
            auto const byte = *it;
            result += ifs;
            result += detail::kHexCharacters[byte >> 4];
            result += detail::kHexCharacters[byte & 0xF];
            ifs = separator;
        }

        return result;
    }

    inline std::string ToHexString(std::uint8_t const* begin, std::size_t length, char const* separator = " ") {
        return ToHexString(begin, begin+length, separator);
    }

    template <std::size_t N>
    inline std::string ToHexString(std::array<std::uint8_t, N> const& array, char const* separator = " ") {
        return ToHexString(array.data(), array.size(), separator);
    }

    template <typename F>
    inline void SplitVisitor(std::string const& text, std::string const& delimiter, F&& visitor) {
        std::size_t begin = 0;
        while (true) {
            auto it = text.find(delimiter, begin);
            if (it == std::string::npos) {
                visitor(text.substr(begin));
                break;
            }

            visitor(text.substr(begin, it-begin));
            begin = it + delimiter.size();
        }
    }

    inline std::string zeroPad(std::string const& text, int length) {
        std::string result;
        result.resize(std::max(length - text.size(), (std::size_t)0), '0');
        return result + text;
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_STRING_H
