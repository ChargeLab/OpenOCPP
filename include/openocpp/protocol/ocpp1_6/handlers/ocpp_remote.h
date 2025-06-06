#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_OCPP_REMOTE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_OCPP_REMOTE_H

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
#include "openocpp/protocol/ocpp1_6/types/message_type.h"
#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/interface/element/websocket_interface.h"

#include <utility>
#include <random>

namespace chargelab::ocpp1_6 {
    class OcppRemote {
    private:
        using OnManagedCall = std::function<bool(std::string const&, ActionId const&)>;

    public:
        explicit OcppRemote(
                WebsocketInterface& websocket_interface,
                std::function<bool()> registration_complete,
                OnManagedCall on_managed_message
        )
            : websocket_interface_(websocket_interface),
              registration_complete_(std::move(registration_complete)),
              on_managed_message_(std::move(on_managed_message))
        {
            std::random_device random_device;
            std::default_random_engine random_engine {random_device()};
            std::uniform_int_distribution<unsigned long> distribution(
                    std::numeric_limits<unsigned long>::min(),
                    std::numeric_limits<unsigned long>::max()
            );
            request_id_ = distribution(random_engine);
        }

    public:
#define CHARGELAB_REQUEST_HANDLER_TEMPLATE(type) \
        std::optional<std::string> send ## type ## Req(const type ## Req &req) { \
            return executeCall(ActionId::k ## type, req); \
        }
        CHARGELAB_PASTE(CHARGELAB_REQUEST_HANDLER_TEMPLATE, CHARGELAB_OCPP_1_6_ACTION_IDS)
#undef CHARGELAB_REQUEST_HANDLER_TEMPLATE

        template <typename T>
        std::optional<std::string> sendCall(T const& req) {
            return executeCall(T::kActionId, req);
        }

        bool sendCall(std::string const& unique_id, ActionId const& action, std::string const& payload) {
            if (!websocket_interface_.isConnected())
                return false;

            if (!registration_complete_()) {
                switch (action) {
                    default:
                        CHARGELAB_LOG_MESSAGE(info) << "Registration not complete - blocking call: " << action;
                        return false;

                        // Allow these requests to be sent while registration is pending
                    case ActionId::kBootNotification:
                        break;
                }
            }

            if (!on_managed_message_(unique_id, action))
                return false;

            websocket_interface_.sendCustom([&](ByteWriterInterface& stream) {
                json::JsonWriter writer {stream};
                writer.StartArray();
                writer.Int((int)MessageType::kCall);
                writer.String(unique_id);
                writer.String(action.to_string()),
                json::WriteValue<common::RawJson>::write_json(writer, {payload});
                writer.EndArray();
            });
            return true;
        }

        bool sendUnmanagedMessage(std::function<void(ByteWriterInterface&)> payload) {
            if (!websocket_interface_.isConnected())
                return false;

            websocket_interface_.sendCustom(std::move(payload));
            return true;
        }

        unsigned long getRequestId() {
            return request_id_++;
        }

    private:
        template <typename T>
        std::optional<std::string> executeCall(ActionId const& action, T const& payload) {
            if (!websocket_interface_.isConnected())
                return std::nullopt;

            if (!registration_complete_()) {
                switch (action) {
                    default:
                        CHARGELAB_LOG_MESSAGE(info) << "Registration not complete - blocking call: " << action;
                        return std::nullopt;

                    // Allow these requests to be sent while registration is pending
                    case ActionId::kBootNotification:
                        break;
                }
            }

            CHARGELAB_TRY {
                auto unique_id = std::to_string(request_id_++);
                if (!on_managed_message_(unique_id, action))
                    return std::nullopt;

                websocket_interface_.sendCustom([&](ByteWriterInterface& stream) {
                    json::JsonWriter writer {stream};
                    writer.StartArray();
                    writer.Int((int)MessageType::kCall);
                    writer.String(unique_id);
                    writer.String(action.to_string()),
                    json::WriteValue<T>::write_json(writer, payload);
                    writer.EndArray();
                });

                return unique_id;
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Failed sending call with: " << e.what();
                return std::nullopt;
            }
        }

    private:
        WebsocketInterface& websocket_interface_;
        std::function<bool()> registration_complete_;
        OnManagedCall on_managed_message_;

        std::atomic<unsigned long> request_id_;
    };
}

#undef CHARGELAB_MERGE_NAMES
#undef CHARGELAB_REQUEST_HANDLER_TEMPLATE

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_OCPP_REMOTE_H
