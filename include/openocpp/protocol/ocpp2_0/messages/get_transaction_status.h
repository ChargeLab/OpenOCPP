#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_TRANSACTION_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_TRANSACTION_STATUS_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetTransactionStatusRequest {
        std::optional<IdentifierStringPrimitive<36>> transactionId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetTransactionStatusRequest, kGetTransactionStatus, transactionId)
    };


    struct GetTransactionStatusResponse {
        std::optional<bool> ongoingIndicator {};
        bool messagesInQueue {};
        CHARGELAB_JSON_INTRUSIVE(GetTransactionStatusResponse, ongoingIndicator, messagesInQueue)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_TRANSACTION_STATUS_H
