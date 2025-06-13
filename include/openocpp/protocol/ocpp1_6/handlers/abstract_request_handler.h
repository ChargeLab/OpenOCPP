#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_REQUEST_HANDLER_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_REQUEST_HANDLER_H

#include "openocpp/protocol/ocpp1_6/messages/authorize.h"
#include "openocpp/protocol/ocpp1_6/messages/boot_notification.h"
#include "openocpp/protocol/ocpp1_6/messages/cancel_reservation.h"
#include "openocpp/protocol/ocpp1_6/messages/change_availability.h"
#include "openocpp/protocol/ocpp1_6/messages/change_configuration.h"
#include "openocpp/protocol/ocpp1_6/messages/clear_cache.h"
#include "openocpp/protocol/ocpp1_6/messages/clear_charging_profile.h"
#include "openocpp/protocol/ocpp1_6/messages/data_transfer.h"
#include "openocpp/protocol/ocpp1_6/messages/diagnostics_status_notification.h"
#include "openocpp/protocol/ocpp1_6/messages/firmware_status_notification.h"
#include "openocpp/protocol/ocpp1_6/messages/get_composite_schedule.h"
#include "openocpp/protocol/ocpp1_6/messages/get_configuration.h"
#include "openocpp/protocol/ocpp1_6/messages/get_diagnostics.h"
#include "openocpp/protocol/ocpp1_6/messages/get_local_list_version.h"
#include "openocpp/protocol/ocpp1_6/messages/heartbeat.h"
#include "openocpp/protocol/ocpp1_6/messages/meter_values.h"
#include "openocpp/protocol/ocpp1_6/messages/remote_start_transaction.h"
#include "openocpp/protocol/ocpp1_6/messages/remote_stop_transaction.h"
#include "openocpp/protocol/ocpp1_6/messages/reserve_now.h"
#include "openocpp/protocol/ocpp1_6/messages/reset.h"
#include "openocpp/protocol/ocpp1_6/messages/send_local_list.h"
#include "openocpp/protocol/ocpp1_6/messages/set_charging_profile.h"
#include "openocpp/protocol/ocpp1_6/messages/start_transaction.h"
#include "openocpp/protocol/ocpp1_6/messages/status_notification.h"
#include "openocpp/protocol/ocpp1_6/messages/stop_transaction.h"
#include "openocpp/protocol/ocpp1_6/messages/trigger_message.h"
#include "openocpp/protocol/ocpp1_6/messages/unlock_connector.h"
#include "openocpp/protocol/ocpp1_6/messages/update_firmware.h"
#include "openocpp/protocol/ocpp1_6/types/call_result.h"
#include "openocpp/protocol/ocpp1_6/handlers/ocpp_remote.h"

#include <variant>

namespace chargelab::ocpp1_6 {
    class AbstractRequestHandler {
    public:
#define CHARGELAB_REQUEST_HANDLER_TEMPLATE(type) \
        virtual std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::type ## Rsp>> on ## type ## Req(ocpp1_6::type ## Req const&) { \
            return std::nullopt; \
        }
        CHARGELAB_PASTE(CHARGELAB_REQUEST_HANDLER_TEMPLATE, CHARGELAB_OCPP_1_6_ACTION_IDS)
#undef CHARGELAB_REQUEST_HANDLER_TEMPLATE

        virtual std::optional<ocpp1_6::ResponseToRequest<common::RawJson>> onCall(
                ocpp1_6::ActionId const& actionId,
                common::RawJson const& payload
        ) {
            (void)actionId;
            (void)payload;
            return std::nullopt;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_REQUEST_HANDLER_H
