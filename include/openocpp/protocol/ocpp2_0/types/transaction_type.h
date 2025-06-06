#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/charging_state_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/reason_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct TransactionType {
        IdentifierStringPrimitive<36> transactionId {};
        std::optional<ChargingStateEnumType> chargingState {};
        std::optional<int> timeSpentCharging {};
        std::optional<ReasonEnumType> stoppedReason {};
        std::optional<int> remoteStartId {};
        CHARGELAB_JSON_INTRUSIVE(TransactionType, transactionId, chargingState, timeSpentCharging, stoppedReason, remoteStartId)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_TRANSACTION_TYPE_H
