#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_DATA_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_DATA_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/id_tag_info.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct AuthorizationData {
        std::optional<IdToken> idTag;
        std::optional<IdTagInfo> idTagInfo;
        CHARGELAB_JSON_INTRUSIVE(AuthorizationData, idTag, idTagInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_DATA_H
