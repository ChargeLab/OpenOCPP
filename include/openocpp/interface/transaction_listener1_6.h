#ifndef OPENOCPP_TRANSACTION_LISTENER1_6_H
#define OPENOCPP_TRANSACTION_LISTENER1_6_H

#include <optional>

#include "openocpp/model/transaction_container1_6.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/protocol/ocpp1_6/types/sampled_value.h"
#include "openocpp/protocol/ocpp1_6/types/reason.h"

namespace chargelab {
    class TransactionListener1_6 {
    public:
        enum Status {
            kStarted,
            kRunning,
            kStopped,
            kPersistedStopCheckpoint
        };

    public:
        virtual ~TransactionListener1_6() = default;
        virtual void onTransactionUpdate(
            Status status,
            int connector_id,
            transaction_module1_6::TransactionContainer const& transaction,
            std::optional<charger::ConnectorStatus> const& connector_status,
            std::optional<std::vector<ocpp1_6::SampledValue>> const& sampled_values,
            std::optional<chargelab::ocpp1_6::Reason> reason = std::nullopt) = 0;
    };
}

#endif //OPENOCPP_TRANSACTION_LISTENER1_6_H