#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/operational_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"
#include "openocpp/protocol/ocpp2_0/types/change_availability_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChangeAvailabilityRequest {
        OperationalStatusEnumType operationalStatus {};
        std::optional<EVSEType> evse {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ChangeAvailabilityRequest, kChangeAvailability, operationalStatus, evse)
    };


    struct ChangeAvailabilityResponse {
        ChangeAvailabilityStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ChangeAvailabilityResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_H
