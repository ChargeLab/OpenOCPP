#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/additional_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct IdTokenType {
        IdentifierStringPrimitive<36> idToken {};
        IdTokenEnumType type {};
        std::optional<std::vector<AdditionalInfoType>> additionalInfo {};
        CHARGELAB_JSON_INTRUSIVE(IdTokenType, idToken, type, additionalInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_TYPE_H
