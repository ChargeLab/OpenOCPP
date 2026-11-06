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

        // Whether OpenOCPP keeps its open transactions when it stops (a restart, or a power loss), instead of ending
        // them with a PowerLoss stop: another charger is the one charging, and does not stop. They are saved while
        // they run, and the next instance continues them with the same transaction ID. Default: no.
        virtual bool keepTransactionsAcrossRestarts() {
            return false;
        }
    };
}