#pragma once

#include <optional>
#include "openocpp/protocol/ocpp2_0/messages/request_start_transaction.h"
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"

namespace chargelab {

    class TransactionController2_0 {
    public:
        virtual ~TransactionController2_0() = default;

        // Whether a pending start request (an authorized idToken, e.g. from an
        // RFID tap or RequestStartTransaction) may start a transaction on the
        // connector; nullopt: on none in particular. While this returns false
        // the request stays pending. Not asked for a transaction started by
        // the EVConnected start point.
        virtual bool isTransactionStartAllowed(const std::optional<ocpp2_0::EVSEType>& evse) = 0;

        // The backend requested a remote start, which OpenOCPP validated.
        // Return false to reject it.
        virtual bool onRequestStartTransaction(const ocpp2_0::RequestStartTransactionRequest& request) = 0;

        // The backend requested to stop the running transaction on the
        // connector, which OpenOCPP is about to stop.
        virtual void onRequestStopTransaction(const std::optional<ocpp2_0::EVSEType>& evse) = 0;
    };
}