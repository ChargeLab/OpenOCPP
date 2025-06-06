#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_IDENTIFIER_STRING_PRIMITIVE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_IDENTIFIER_STRING_PRIMITIVE_H

#include "openocpp/protocol/common/small_string.h"
#include "openocpp/helpers/json.h"
#include "openocpp/helpers/string.h"

#include <string>
#include <utility>

namespace chargelab::ocpp2_0 {
    template <int MaxLength>
    class IdentifierStringPrimitive {
    public:
        using this_type = IdentifierStringPrimitive<MaxLength>;
        
    public:
        IdentifierStringPrimitive() {}
        IdentifierStringPrimitive(const this_type& other) = default;
        this_type& operator=(const this_type& other) = default;

        template <int N>
        IdentifierStringPrimitive(char const (&literal)[N]) : value_(literal) {}
        IdentifierStringPrimitive(std::string const& text) : value_(text) {}

        static bool read_json(json::JsonReader &reader, this_type &value) {
            std::string text;
            if (!json::ReadValue<std::string>::read_json(reader, text))
                return false;

            value.value_ = std::move(text);
            return true;
        }

        static void write_json(::chargelab::json::JsonWriter &writer, this_type const &value) {
            json::WritePrimitive<std::string>::write_json(writer, value.value_.value());
        }

        bool operator==(const IdentifierStringPrimitive <MaxLength> &rhs) const {
            return string::EqualsIgnoreCaseAscii(value_.value(), rhs.value_.value());
        }

        bool operator!=(const IdentifierStringPrimitive <MaxLength> &rhs) const {
            return !operator==(rhs);
        }

        [[nodiscard]] std::string value() const {
            return value_.value();
        }

    private:
        SmallString value_ {};
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_IDENTIFIER_STRING_PRIMITIVE_H
