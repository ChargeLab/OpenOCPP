#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_STOP_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_STOP_TRANSACTION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/request_start_stop_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct RequestStopTransactionRequest {
        IdentifierStringPrimitive<36> transactionId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(RequestStopTransactionRequest, kRequestStopTransaction, transactionId)
    };


    struct RequestStopTransactionResponse {
        RequestStartStopStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(RequestStopTransactionResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REQUEST_STOP_TRANSACTION_H
