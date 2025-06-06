#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_SEND_LOCAL_LIST_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_SEND_LOCAL_LIST_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/authorization_data.h"
#include "openocpp/protocol/ocpp1_6/types/update_type.h"
#include "openocpp/protocol/ocpp1_6/types/update_status.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct SendLocalListReq {
        int listVersion;
        std::optional<std::vector<AuthorizationData>> localAuthorizationList;
        UpdateType updateType;
        CHARGELAB_JSON_INTRUSIVE_CALL(SendLocalListReq, kSendLocalList, listVersion, localAuthorizationList, updateType)
    };

    struct SendLocalListRsp {
        UpdateStatus status;
        CHARGELAB_JSON_INTRUSIVE(SendLocalListRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_SEND_LOCAL_LIST_H
