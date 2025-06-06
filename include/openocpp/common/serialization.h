#ifndef CHARGELAB_OPEN_FIRMWARE_SERIALIZATION_H
#define CHARGELAB_OPEN_FIRMWARE_SERIALIZATION_H

#include <type_traits>
#include <optional>
#include <string>
#include <cstring>

namespace chargelab {
    // Note: this file provides a number of very simple binary serialization wrappers that are used in certain contexts
    // where JSON was too expensive (such as maintaining a large number of historic messages as a compressed stream).

    template <typename T>
    typename std::enable_if<std::is_trivial<T>::value, std::optional<std::size_t>>::type
    readPrimitive(std::string_view const& text, std::optional<std::size_t> index, T& out) {
        if (!index.has_value())
            return std::nullopt;
        if (index.value() + sizeof(out) > text.size())
            return std::nullopt;

        std::memcpy(&out, text.data() + index.value(), sizeof(out));
        return index.value() + sizeof(out);
    }

    template <typename T>
    typename std::enable_if<std::is_trivial<typename T::Value>::value, std::optional<std::size_t>>::type
    readPrimitive(std::string_view const& text, std::optional<std::size_t> index, T& out) {
        using raw_type = typename T::Value;
        if (!index.has_value())
            return std::nullopt;
        if (index.value() + sizeof(raw_type{}) > text.size())
            return std::nullopt;

        raw_type raw;
        std::memcpy(&raw, text.data() + index.value(), sizeof(raw));
        out = raw;
        return index.value() + sizeof(raw);
    }

    inline std::optional<std::size_t> readPrimitive(std::string_view const& text, std::optional<std::size_t> index, std::string& out) {
        int32_t size = 0;
        index = readPrimitive(text, index, size);
        if (!index.has_value() || size < 0)
            return std::nullopt;
        if (index.value() + size > text.size())
            return std::nullopt;

        out.resize(size);
        std::memcpy(out.data(), text.data() + index.value(), size);
        return index.value() + size;
    }

    template <typename T>
    std::optional<std::size_t> readPrimitive(std::string_view const& text, std::optional<std::size_t> index, std::optional<T>& out) {
        bool present = false;
        index = readPrimitive(text, index, present);
        if (!index.has_value())
            return std::nullopt;

        if (present) {
            T value;
            auto result = readPrimitive(text, index, value);
            out = std::move(value);
            return result;
        } else {
            return index;
        }
    }

    template <typename T>
    typename std::enable_if<std::is_trivial<T>::value>::type
    writePrimitive(std::string& text, T const& in) {
        auto index = text.size();
        text.resize(index + sizeof(in));
        std::memcpy(text.data() + index, &in, sizeof(in));
    }

    template <typename T>
    typename std::enable_if<std::is_trivial<typename T::Value>::value>::type
    writePrimitive(std::string& text, T const& in) {
        using raw_type = typename T::Value;
        raw_type raw = in;

        auto index = text.size();
        text.resize(index + sizeof(raw));
        std::memcpy(text.data() + index, &raw, sizeof(raw));
    }

    inline void writePrimitive(std::string& text, std::string const& in) {
        writePrimitive(text, (int32_t)in.size());

        auto index = text.size();
        text.resize(index + in.size());
        std::memcpy(text.data() + index, in.data(), in.size());
    }

    template <typename T>
    void writePrimitive(std::string& text, std::optional<T> const& in) {
        if (in.has_value()) {
            writePrimitive(text, true);
            writePrimitive(text, in.value());
        } else {
            writePrimitive(text, false);
        }
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_SERIALIZATION_H
