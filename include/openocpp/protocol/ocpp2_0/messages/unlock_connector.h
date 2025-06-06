#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UNLOCK_CONNECTOR_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UNLOCK_CONNECTOR_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/unlock_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct UnlockConnectorRequest {
        int evseId {};
        int connectorId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(UnlockConnectorRequest, kUnlockConnector, evseId, connectorId)
    };


    struct UnlockConnectorResponse {
        UnlockStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(UnlockConnectorResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UNLOCK_CONNECTOR_H
