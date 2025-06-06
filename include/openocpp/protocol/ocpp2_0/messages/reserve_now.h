#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_RESERVE_NOW_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_RESERVE_NOW_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/connector_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/reserve_now_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ReserveNowRequest {
        int id {};
        DateTime expiryDateTime {};
        std::optional<ConnectorEnumType> connectorType {};
        std::optional<int> evseId {};
        IdTokenType idToken {};
        std::optional<IdTokenType> groupIdToken {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ReserveNowRequest, kReserveNow, id, expiryDateTime, connectorType, evseId, idToken, groupIdToken)
    };


    struct ReserveNowResponse {
        ReserveNowStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ReserveNowResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_RESERVE_NOW_H
