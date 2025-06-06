#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_START_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_START_TRANSACTION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/request_start_stop_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct RequestStartTransactionRequest {
        std::optional<int> evseId {};
        int remoteStartId {};
        IdTokenType idToken {};
        std::optional<ChargingProfileType> chargingProfile {};
        std::optional<IdTokenType> groupIdToken {};
        CHARGELAB_JSON_INTRUSIVE_CALL(RequestStartTransactionRequest, kRequestStartTransaction, evseId, remoteStartId, idToken, chargingProfile, groupIdToken)
    };


    struct RequestStartTransactionResponse {
        RequestStartStopStatusEnumType status {};
        std::optional<IdentifierStringPrimitive<36>> transactionId {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(RequestStartTransactionResponse, status, transactionId, statusInfo)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_START_TRANSACTION_H
