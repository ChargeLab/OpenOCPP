#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/charging_profile_purpose_type.h"
#include "openocpp/protocol/ocpp1_6/types/charging_profile_kind_type.h"
#include "openocpp/protocol/ocpp1_6/types/recurrency_kind_type.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/charging_schedule.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct ChargingProfile {
        int chargingProfileId;
        std::optional<int> transactionId;
        int stackLevel;
        ChargingProfilePurposeType chargingProfilePurpose;
        ChargingProfileKindType chargingProfileKind;
        std::optional<RecurrencyKindType> recurrencyKind;
        std::optional<DateTime> validFrom;
        std::optional<DateTime> validTo;
        ChargingSchedule chargingSchedule;
        CHARGELAB_JSON_INTRUSIVE(ChargingProfile, chargingProfileId, transactionId, stackLevel, chargingProfilePurpose, chargingProfileKind, recurrencyKind, validFrom, validTo, chargingSchedule)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_H
