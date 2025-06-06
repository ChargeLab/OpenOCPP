#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_UNLOCK_CONNECTOR_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_UNLOCK_CONNECTOR_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/unlock_status.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct UnlockConnectorReq {
        int connectorId;
        CHARGELAB_JSON_INTRUSIVE_CALL(UnlockConnectorReq, kUnlockConnector, connectorId)
    };

    struct UnlockConnectorRsp {
        UnlockStatus status;
        CHARGELAB_JSON_INTRUSIVE(UnlockConnectorRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_UNLOCK_CONNECTOR_H
