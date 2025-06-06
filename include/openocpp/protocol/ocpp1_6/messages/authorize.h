#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/protocol/ocpp1_6/types/id_tag_info.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct AuthorizeReq {
        IdToken idTag;
        CHARGELAB_JSON_INTRUSIVE_CALL(AuthorizeReq, kAuthorize, idTag)
    };

    struct AuthorizeRsp {
        IdTagInfo idTagInfo;
        CHARGELAB_JSON_INTRUSIVE(AuthorizeRsp, idTagInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZE_H
