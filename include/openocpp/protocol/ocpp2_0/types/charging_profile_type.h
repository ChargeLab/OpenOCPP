#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/charging_profile_purpose_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_kind_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/recurrency_kind_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/charging_schedule_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingProfileType {
        int id {};
        int stackLevel {};
        ChargingProfilePurposeEnumType chargingProfilePurpose {};
        ChargingProfileKindEnumType chargingProfileKind {};
        std::optional<RecurrencyKindEnumType> recurrencyKind {};
        std::optional<DateTime> validFrom {};
        std::optional<DateTime> validTo {};
        std::optional<IdentifierStringPrimitive<36>> transactionId {};
        std::vector<ChargingScheduleType> chargingSchedule {};
        CHARGELAB_JSON_INTRUSIVE(ChargingProfileType, id, stackLevel, chargingProfilePurpose, chargingProfileKind, recurrencyKind, validFrom, validTo, transactionId, chargingSchedule)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_TYPE_H
