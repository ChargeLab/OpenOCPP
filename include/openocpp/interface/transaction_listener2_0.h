#ifndef OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_H
#define OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_H

#include "openocpp/model/transaction_container2_0.h"
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"
#include "openocpp/protocol/ocpp2_0/types/sampled_value_type.h"
#include "openocpp/interface/station_interface.h"

namespace chargelab {
    class TransactionListener2_0 {
    public:
        enum Status {
            kStarted,
            kRunning,
            kStopped,
            kPersistedStopCheckpoint
        };

    public:
        virtual ~TransactionListener2_0() = default;

        virtual void onTransactionUpdate(
            Status status,
            std::optional<ocpp2_0::EVSEType> const& evse,
            chargelab::transaction_module2_0::TransactionContainer const& transaction,
            std::optional<charger::ConnectorStatus> const& connector_status,
            std::optional<std::vector<ocpp2_0::SampledValueType>> const& sampled_values
        ) = 0;
    };
}


#endif //OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_H