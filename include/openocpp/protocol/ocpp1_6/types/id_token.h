#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ID_TOKEN_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ID_TOKEN_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/authorization_status.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

#include <string>
#include <utility>

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class IdToken {
    public:
        IdToken() {}
        IdToken(std::string value) : value_(std::move(value)) {}
        IdToken(const IdToken& other) = default;
        IdToken& operator=(const IdToken& other) = default;
        CHARGELAB_JSON_AS_PRIMITIVE(IdToken, value_)

        [[nodiscard]] std::string const& value() const {
            return value_;
        }

    private:
        std::string value_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ID_TOKEN_H
