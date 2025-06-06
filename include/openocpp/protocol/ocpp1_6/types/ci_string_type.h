#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CI_STRING_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CI_STRING_TYPE_H

#include <string>
#include <utility>

#include "openocpp/helpers/json.h"
#include "openocpp/helpers/string.h"

namespace chargelab::ocpp1_6 {
    template <int MaxLength>
    class CaseInsensitiveString {
    public:
        using this_type = CaseInsensitiveString<MaxLength>;
        
    public:
        CaseInsensitiveString() {}
        CaseInsensitiveString(std::string value) : value_(std::move(value)) {}
        CaseInsensitiveString(const this_type& other) = default;
        this_type& operator=(const this_type& other) = default;

        [[nodiscard]] std::string const& value() const {
            return value_;
        }

        static bool read_json(json::JsonReader &reader, this_type &value) {
            return json::ReadPrimitive<std::string>::read_json(reader, value.value_);
        }

        static void write_json(::chargelab::json::JsonWriter &writer, this_type const &value) {
            json::WritePrimitive<std::string>::write_json(writer, value.value_);
        }

        bool operator==(this_type const& rhs) const {
            return string::EqualsIgnoreCaseAscii(value_, rhs.value_);
        }

        bool operator!=(this_type const& rhs) const {
            return !operator==(rhs);
        }

    private:
        std::string value_ {};
    };

    using CiString20Type = CaseInsensitiveString<20>;
    using CiString25Type = CaseInsensitiveString<25>;
    using CiString50Type = CaseInsensitiveString<50>;
    using CiString255Type = CaseInsensitiveString<255>;
    using CiString500Type = CaseInsensitiveString<500>;
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CI_STRING_TYPE_H
