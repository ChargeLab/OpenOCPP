#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_TRANSACTION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/protocol/ocpp1_6/types/charging_profile.h"
#include "openocpp/protocol/ocpp1_6/types/remote_start_stop_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct RemoteStartTransactionReq {
        std::optional<int> connectorId;
        IdToken idTag;
        std::optional<ChargingProfile> chargingProfile;
        CHARGELAB_JSON_INTRUSIVE_CALL(RemoteStartTransactionReq, kRemoteStartTransaction, connectorId, idTag, chargingProfile)
    };

    struct RemoteStartTransactionRsp {
        RemoteStartStopStatus status;
        CHARGELAB_JSON_INTRUSIVE(RemoteStartTransactionRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_TRANSACTION_H
