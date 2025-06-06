#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ANY_URI_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ANY_URI_H

#include <string>

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class AnyUri {
    public:
        AnyUri() {}
        explicit AnyUri(std::string value) : value_(std::move(value)) {}
        AnyUri(const AnyUri& other) = default;
        AnyUri& operator=(const AnyUri& other) = default;
        CHARGELAB_JSON_AS_PRIMITIVE(AnyUri, value_)

        [[nodiscard]] std::string const& value() const {
            return value_;
        }

    private:
        std::string value_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ANY_URI_H
