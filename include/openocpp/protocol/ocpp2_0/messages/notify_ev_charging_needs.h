#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_NEEDS_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_NEEDS_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_needs_type.h"
#include "openocpp/protocol/ocpp2_0/types/notify_ev_charging_needs_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyEVChargingNeedsRequest {
        std::optional<int> maxScheduleTuples {};
        int evseId {};
        ChargingNeedsType chargingNeeds {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyEVChargingNeedsRequest, kNotifyEVChargingNeeds, maxScheduleTuples, evseId, chargingNeeds)
    };


    struct NotifyEVChargingNeedsResponse {
        NotifyEVChargingNeedsStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(NotifyEVChargingNeedsResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_NEEDS_H
