#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_CHARGING_PROFILES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_CHARGING_PROFILES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_limit_source_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_type.h"
#include "openocpp/protocol/common/vector.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ReportChargingProfilesRequest {
        int requestId {};
        ChargingLimitSourceEnumType chargingLimitSource {};
        std::optional<bool> tbc {};
        int evseId {};
        Vector<ChargingProfileType, false> chargingProfile {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ReportChargingProfilesRequest, kReportChargingProfiles, requestId, chargingLimitSource, tbc, evseId, chargingProfile)
    };


    struct ReportChargingProfilesResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(ReportChargingProfilesResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_CHARGING_PROFILES_H
