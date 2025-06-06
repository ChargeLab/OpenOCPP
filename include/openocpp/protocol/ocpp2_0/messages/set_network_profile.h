#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_NETWORK_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_NETWORK_PROFILE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/network_connection_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/set_network_profile_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetNetworkProfileRequest {
        int configurationSlot {};
        NetworkConnectionProfileType connectionData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetNetworkProfileRequest, kSetNetworkProfile, configurationSlot, connectionData)
    };


    struct SetNetworkProfileResponse {
        SetNetworkProfileStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetNetworkProfileResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_NETWORK_PROFILE_H
