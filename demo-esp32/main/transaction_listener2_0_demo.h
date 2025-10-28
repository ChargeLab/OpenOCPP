#ifndef OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_DEMO_H
#define OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_DEMO_H

#include <memory>
#include <optional>
#include "openocpp/interface/transaction_listener2_0.h"
#include "openocpp/interface/component/system_interface.h"

namespace chargelab {
    class TransactionListener2_0Demo : public TransactionListener2_0 {
    public:
        TransactionListener2_0Demo(std::shared_ptr<SystemInterface> system) :
            system_(std::move(system)) {
        }
        void onTransactionUpdate(
            Status status,
            std::optional<ocpp2_0::EVSEType> const& evse,
            chargelab::transaction_module2_0::TransactionContainer const& transaction,
            std::optional<charger::ConnectorStatus> const& connector_status,
            std::optional<std::vector<ocpp2_0::SampledValueType>> const& sampled_values
        ) override {
            if (status == TransactionListener2_0::Status::kRunning) {
                auto now = system_->steadyClockNow();
                if (last_log_time_.has_value() && now - last_log_time_.value() < 10*1000) {
                    return;
                }

                last_log_time_ = now;
            }
            CHARGELAB_LOG_MESSAGE(debug) << "transaction2_0 status: " << status;
        }
    private:
        std::shared_ptr<SystemInterface> system_;
        std::optional<chargelab::SteadyPointMillis> last_log_time_ = std::nullopt;
    };
}


#endif //OPENOCPP_INTERNAL_TRANSACTION_LISTENER2_0_DEMO_H