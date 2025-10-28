#ifndef OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_H
#define OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_H

#include "openocpp/model/transaction_container1_6.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/protocol/ocpp1_6/types/sampled_value.h"

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
            std::optional<std::vector<ocpp1_6::SampledValue>> const& sampled_values) = 0;
    };
}

#endif //OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_H