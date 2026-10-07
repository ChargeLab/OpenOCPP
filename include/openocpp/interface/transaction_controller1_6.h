#pragma once

#include <string>
#include <optional>
#include "openocpp/protocol/ocpp1_6/types/reason.h"
#include "openocpp/protocol/ocpp1_6/messages/remote_start_transaction.h"

namespace chargelab {

    class TransactionController1_6 {
    public:
        virtual ~TransactionController1_6() = default;

        virtual bool isTransactionStartAllowed(
            int connector_id,
            const std::string& id_tag) = 0;

        virtual std::optional<chargelab::ocpp1_6::Reason> getTransactionStopReason(
            int connector_id) = 0;

        // Called when a RemoteStartTransaction.req from the backend has passed
        // OpenOCPP's own checks, before it is accepted. Returning false rejects
        // it.
        virtual bool onRemoteStartTransaction(
            const ocpp1_6::RemoteStartTransactionReq& request) {
            return true;
        }
    };
}