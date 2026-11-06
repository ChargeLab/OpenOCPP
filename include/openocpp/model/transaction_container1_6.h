#ifndef OPENOCPP_TRANSACTION_CONTAINER1_6_H
#define OPENOCPP_TRANSACTION_CONTAINER1_6_H

#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <optional>

#include "openocpp/helpers/json.h"
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

            // The ID another party gave the transaction, e.g. the charger OpenOCPP is run for (it is kept with the
            // transaction across a restart, see TransactionModule1_6::setExternalTransactionId())
            std::optional<int> external_transaction_id = std::nullopt;

            // Restored after a restart, and not handled until the station knows the connector again
            bool restored = false;

            // The meter reading kept with a restored transaction. The energy register does not decrease, so the
            // reading of the stop is at least this one, also before the station has a reading again.
            std::optional<double> restored_meter_watt_hours = std::nullopt;
        };

        // What is kept of a transaction across a restart, see TransactionModule1_6::saveForRestart(). The group ID is
        // a decimal string.
        struct SavedTransaction {
            int connector_id {};
            std::string group_id {};
            std::string id_tag {};
            std::string start_transaction_request_id {};
            std::optional<int> transaction_id {};
            std::optional<int> external_transaction_id {};
            double meter_watt_hours {};
            CHARGELAB_JSON_INTRUSIVE(SavedTransaction, connector_id, group_id, id_tag, start_transaction_request_id,
                                     transaction_id, external_transaction_id, meter_watt_hours)
        };

        // An open transaction on a connector, see TransactionModule1_6::activeTransactions().
        struct ActiveTransactionInfo {
            int connector_id;
            std::string id_tag;
            std::optional<int> external_transaction_id;
        };
    }
}

#endif //OPENOCPP_TRANSACTION_CONTAINER1_6_H