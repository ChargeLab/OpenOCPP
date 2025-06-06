#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_EVSE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_EVSE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct EVSEType {
        int id {};
        std::optional<int> connectorId {};
        CHARGELAB_JSON_INTRUSIVE(EVSEType, id, connectorId)

        bool operator<(EVSEType const& rhs) const {
            if (id < rhs.id)
                return true;
            if (id > rhs.id)
                return false;

            return connectorId < rhs.connectorId;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_EVSE_TYPE_H
