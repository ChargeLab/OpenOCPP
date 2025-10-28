#ifndef OPENOCPP_INTERNAL_TRANSACTION_CONTAINER2_0_H
#define OPENOCPP_INTERNAL_TRANSACTION_CONTAINER2_0_H

#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <optional>

#include "openocpp/model/system_types.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_state_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/meter_value_type.h"

namespace chargelab {
    namespace transaction_module2_0 {
        struct TransactionContainer {
            uint64_t transaction_id;
            chargelab::SteadyPointMillis start_ts;

            std::optional<chargelab::ocpp2_0::IdTokenType> id_token;
            std::optional<chargelab::ocpp2_0::IdTokenType> group_id_token;
            std::string start_transaction_request_id;
            std::optional<chargelab::ocpp2_0::ChargingProfileType> charging_profile;
            bool authorized;
            bool deauthorized;
            bool plugged_in;

            chargelab::SystemTimeMillis last_sampled_reading_timestamp;
            chargelab::SystemTimeMillis last_sampled_ended_meter_values_timestamp;
            chargelab::SystemTimeMillis last_clock_aligned_reading_timestamp;
            chargelab::SystemTimeMillis last_clock_aligned_ended_meter_values_timestamp;

            std::optional<chargelab::ocpp2_0::ChargingStateEnumType> last_charging_status = std::nullopt;
            std::vector<chargelab::ocpp2_0::MeterValueType> ended_meter_values {};
        };
    }
}

#endif //OPENOCPP_INTERNAL_TRANSACTION_CONTAINER2_0_H