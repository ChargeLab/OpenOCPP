#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_REQUEST_HANDLER_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_REQUEST_HANDLER_H

#include "openocpp/protocol/ocpp2_0/messages/authorize.h"
#include "openocpp/protocol/ocpp2_0/messages/boot_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/cancel_reservation.h"
#include "openocpp/protocol/ocpp2_0/messages/certificate_signed.h"
#include "openocpp/protocol/ocpp2_0/messages/change_availability.h"
#include "openocpp/protocol/ocpp2_0/messages/clear_cache.h"
#include "openocpp/protocol/ocpp2_0/messages/clear_charging_profile.h"
#include "openocpp/protocol/ocpp2_0/messages/clear_display_message.h"
#include "openocpp/protocol/ocpp2_0/messages/clear_variable_monitoring.h"
#include "openocpp/protocol/ocpp2_0/messages/cleared_charging_limit.h"
#include "openocpp/protocol/ocpp2_0/messages/cost_updated.h"
#include "openocpp/protocol/ocpp2_0/messages/customer_information.h"
#include "openocpp/protocol/ocpp2_0/messages/data_transfer.h"
#include "openocpp/protocol/ocpp2_0/messages/delete_certificate.h"
#include "openocpp/protocol/ocpp2_0/messages/firmware_status_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/get_15118_ev_certificate.h"
#include "openocpp/protocol/ocpp2_0/messages/get_base_report.h"
#include "openocpp/protocol/ocpp2_0/messages/get_certificate_status.h"
#include "openocpp/protocol/ocpp2_0/messages/get_charging_profiles.h"
#include "openocpp/protocol/ocpp2_0/messages/get_composite_schedule.h"
#include "openocpp/protocol/ocpp2_0/messages/get_display_messages.h"
#include "openocpp/protocol/ocpp2_0/messages/get_installed_certificate_ids.h"
#include "openocpp/protocol/ocpp2_0/messages/get_local_list_version.h"
#include "openocpp/protocol/ocpp2_0/messages/get_log.h"
#include "openocpp/protocol/ocpp2_0/messages/get_monitoring_report.h"
#include "openocpp/protocol/ocpp2_0/messages/get_report.h"
#include "openocpp/protocol/ocpp2_0/messages/get_transaction_status.h"
#include "openocpp/protocol/ocpp2_0/messages/get_variables.h"
#include "openocpp/protocol/ocpp2_0/messages/heartbeat.h"
#include "openocpp/protocol/ocpp2_0/messages/install_certificate.h"
#include "openocpp/protocol/ocpp2_0/messages/log_status_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/meter_values.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_charging_limit.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_customer_information.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_display_messages.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_ev_charging_needs.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_ev_charging_schedule.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_event.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_monitoring_report.h"
#include "openocpp/protocol/ocpp2_0/messages/notify_report.h"
#include "openocpp/protocol/ocpp2_0/messages/publish_firmware.h"
#include "openocpp/protocol/ocpp2_0/messages/publish_firmware_status_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/report_charging_profiles.h"
#include "openocpp/protocol/ocpp2_0/messages/request_start_transaction.h"
#include "openocpp/protocol/ocpp2_0/messages/request_stop_transaction.h"
#include "openocpp/protocol/ocpp2_0/messages/reservation_status_update.h"
#include "openocpp/protocol/ocpp2_0/messages/reserve_now.h"
#include "openocpp/protocol/ocpp2_0/messages/reset.h"
#include "openocpp/protocol/ocpp2_0/messages/security_event_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/send_local_list.h"
#include "openocpp/protocol/ocpp2_0/messages/set_charging_profile.h"
#include "openocpp/protocol/ocpp2_0/messages/set_display_message.h"
#include "openocpp/protocol/ocpp2_0/messages/set_monitoring_base.h"
#include "openocpp/protocol/ocpp2_0/messages/set_monitoring_level.h"
#include "openocpp/protocol/ocpp2_0/messages/set_network_profile.h"
#include "openocpp/protocol/ocpp2_0/messages/set_variable_monitoring.h"
#include "openocpp/protocol/ocpp2_0/messages/set_variables.h"
#include "openocpp/protocol/ocpp2_0/messages/sign_certificate.h"
#include "openocpp/protocol/ocpp2_0/messages/status_notification.h"
#include "openocpp/protocol/ocpp2_0/messages/transaction_event.h"
#include "openocpp/protocol/ocpp2_0/messages/trigger_message.h"
#include "openocpp/protocol/ocpp2_0/messages/unlock_connector.h"
#include "openocpp/protocol/ocpp2_0/messages/unpublish_firmware.h"
#include "openocpp/protocol/ocpp2_0/messages/update_firmware.h"
#include "openocpp/protocol/ocpp2_0/types/call_result.h"
#include "openocpp/protocol/ocpp2_0/handlers/ocpp_remote.h"

#include <variant>

namespace chargelab::ocpp2_0 {
    class AbstractRequestHandler {
    public:
#define CHARGELAB_REQUEST_HANDLER_TEMPLATE(TYPE) \
        virtual std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TYPE ## Response>> on ## TYPE ## Req(ocpp2_0::TYPE ## Request const&) { \
            return std::nullopt; \
        }
        CHARGELAB_PASTE(CHARGELAB_REQUEST_HANDLER_TEMPLATE, CHARGELAB_OCPP_2_0_ACTION_IDS)
#undef CHARGELAB_REQUEST_HANDLER_TEMPLATE

        virtual std::optional<ocpp2_0::ResponseToRequest<common::RawJson>> onCall(
                ocpp2_0::ActionId const& actionId,
                common::RawJson const& payload
        ) {
            (void)actionId;
            (void)payload;
            return std::nullopt;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_REQUEST_HANDLER_H
