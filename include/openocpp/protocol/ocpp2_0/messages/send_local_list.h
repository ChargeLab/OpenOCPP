#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/update_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/authorization_data.h"
#include "openocpp/protocol/ocpp2_0/types/send_local_list_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SendLocalListRequest {
        int versionNumber {};
        UpdateEnumType updateType {};
        std::optional<std::vector<AuthorizationData>> localAuthorizationList {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SendLocalListRequest, kSendLocalList, versionNumber, updateType, localAuthorizationList)
    };


    struct SendLocalListResponse {
        SendLocalListStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SendLocalListResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_H
