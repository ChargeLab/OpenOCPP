#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_INFO_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_INFO_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/authorization_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/message_content_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct IdTokenInfoType {
        AuthorizationStatusEnumType status {};
        std::optional<DateTime> cacheExpiryDateTime {};
        std::optional<int> chargingPriority {};
        std::optional<StringPrimitive<8>> language1 {};
        std::optional<std::vector<int>> evseId {};
        std::optional<StringPrimitive<8>> language2 {};
        std::optional<IdTokenType> groupIdToken {};
        std::optional<MessageContentType> personalMessage {};
        CHARGELAB_JSON_INTRUSIVE(IdTokenInfoType, status, cacheExpiryDateTime, chargingPriority, language1, evseId, language2, groupIdToken, personalMessage)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_INFO_TYPE_H
