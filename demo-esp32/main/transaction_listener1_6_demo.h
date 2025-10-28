#ifndef OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_DEMO_H
#define OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_DEMO_H

#include <memory>
#include <optional>
#include "openocpp/interface/transaction_listener1_6.h"
#include "openocpp/interface/component/system_interface.h"

namespace chargelab {
    class TransactionListener1_6Demo : public TransactionListener1_6 {
    public:
        TransactionListener1_6Demo(std::shared_ptr<SystemInterface> system) :
            system_(std::move(system)) {

        }
        void onTransactionUpdate(
            Status status,
            int connector_id,
            transaction_module1_6::TransactionContainer const& transaction,
            std::optional<charger::ConnectorStatus> const& connector_status,
            std::optional<std::vector<ocpp1_6::SampledValue>> const& sampled_values
        ) override {
            if (status == TransactionListener1_6::Status::kRunning) {
                auto now = system_->steadyClockNow();
                if (last_log_time_.has_value() && now - last_log_time_.value() < 10*1000) {
                    return;
                }

                last_log_time_ = now;
            }
                
            CHARGELAB_LOG_MESSAGE(debug) << "transaction1_6 status: " << status;
        }
    private:
        std::shared_ptr<SystemInterface> system_;
        std::optional<chargelab::SteadyPointMillis> last_log_time_ = std::nullopt;
    };
}


#endif //OPENOCPP_INTERNAL_TRANSACTION_LISTENER1_6_DEMO_H