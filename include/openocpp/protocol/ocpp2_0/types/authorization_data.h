#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZATION_DATA_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZATION_DATA_H

#include "openocpp/protocol/ocpp2_0/types/id_token_info_type.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct AuthorizationData {
        std::optional<IdTokenInfoType> idTokenInfo{};
        IdTokenType idToken{};
        CHARGELAB_JSON_INTRUSIVE(AuthorizationData, idTokenInfo, idToken)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZATION_DATA_H