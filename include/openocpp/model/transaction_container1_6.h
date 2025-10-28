#ifndef OPENOCPP_INTERNAL_TRANSACTION_CONTAINER1_6_H
#define OPENOCPP_INTERNAL_TRANSACTION_CONTAINER1_6_H

#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <optional>

#include "openocpp/model/system_types.h"

namespace chargelab {
    namespace transaction_module1_6 {
        struct TransactionContainer {
            int connector_id;
            uint64_t group_id;
            std::string id_tag;
            std::string start_transaction_request_id;
            chargelab::SteadyPointMillis transaction_start_timestamp;

            std::optional<int> transaction_id = std::nullopt;
            std::optional<chargelab::SteadyPointMillis> last_reading_timestamp = std::nullopt;
            std::optional<chargelab::SteadyPointMillis> last_clock_aligned_reading_timestamp = std::nullopt;
            bool connector_unplugged_after_transaction_ended = false;
        };
    }
}

#endif //OPENOCPP_INTERNAL_TRANSACTION_CONTAINER1_6_H