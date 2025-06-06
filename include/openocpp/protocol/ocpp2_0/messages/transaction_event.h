#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_EVENT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_EVENT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/transaction_event_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/trigger_reason_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/transaction_type.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/meter_value_type.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_info_type.h"
#include "openocpp/protocol/ocpp2_0/types/message_content_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct TransactionEventRequest {
        TransactionEventEnumType eventType {};
        DateTime timestamp {};
        TriggerReasonEnumType triggerReason {};
        int seqNo {};
        std::optional<bool> offline {};
        std::optional<int> numberOfPhasesUsed {};
        std::optional<int> cableMaxCurrent {};
        std::optional<int> reservationId {};
        TransactionType transactionInfo {};
        std::optional<IdTokenType> idToken {};
        std::optional<EVSEType> evse {};
        std::optional<std::vector<MeterValueType>> meterValue {};
        CHARGELAB_JSON_INTRUSIVE_CALL(TransactionEventRequest, kTransactionEvent, eventType, timestamp, triggerReason, seqNo, offline, numberOfPhasesUsed, cableMaxCurrent, reservationId, transactionInfo, idToken, evse, meterValue)
    };


    struct TransactionEventResponse {
        std::optional<double> totalCost {};
        std::optional<int> chargingPriority {};
        std::optional<IdTokenInfoType> idTokenInfo {};
        std::optional<MessageContentType> updatedPersonalMessage {};
        CHARGELAB_JSON_INTRUSIVE(TransactionEventResponse, totalCost, chargingPriority, idTokenInfo, updatedPersonalMessage)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_EVENT_H
