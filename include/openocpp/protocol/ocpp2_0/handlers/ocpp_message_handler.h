#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_MESSAGE_HANDLER_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_MESSAGE_HANDLER_H
 
#include "openocpp/protocol/ocpp2_0/handlers/abstract_service.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_request_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_response_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/ocpp_remote.h"
#include "openocpp/protocol/ocpp2_0/types/message_type.h"
#include "openocpp/protocol/ocpp2_0/types/error_code.h"
#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/call_result.h"
#include "openocpp/interface/element/websocket_interface.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/module/abstract_module.h"
#include "openocpp/common/logging.h"
#include "openocpp/common/ring_buffer.h"
#include "openocpp/helpers/string.h"
#include "openocpp/common/settings.h"

#include <memory>
#include <utility>
#include <random>
#include <vector>

namespace chargelab::ocpp2_0 {
    namespace detail {
        struct PendingCall {
            std::string unique_id;
            ActionId action_id;
            SteadyPointMillis timestamp;

            CHARGELAB_JSON_INTRUSIVE(PendingCall, unique_id, action_id, timestamp)
        };
    }

    class OcppMessageHandler {
    private:
        static constexpr const int kMaxPendingCalls = 5;
        static constexpr const int kMaxMessageProcessedPerStep = 4;
        static constexpr const int kMaxCallDelayMillis = 5000;

    public:
        OcppMessageHandler(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<SystemInterface> system,
                std::vector<std::shared_ptr<AbstractModuleInterface>> modules,
                std::function<bool()> registrationComplete
        )
                : settings_(std::move(settings)),
                  system_(std::move(system)),
                  modules_(std::move(modules)),
                  registration_complete_(std::move(registrationComplete))
        {
            updatePointers();
        }

        void runStep(std::shared_ptr<WebsocketInterface> const& websocket) {
            for (auto& x : pure_services_)
                x->runUnconditionally();

            if (websocket == nullptr) {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected null websocket pointer provided";
                return;
            }

            auto const subprotocol = websocket->getSubprotocol();
            if (!subprotocol.has_value() || !string::EqualsIgnoreCaseAscii(subprotocol.value(), "ocpp2.0.1")) {
                CHARGELAB_LOG_MESSAGE(trace) << "Skipping OCPP 2.0.1 message handler based on websocket subprotocol: " << subprotocol;
                return;
            }

            for (int i=0; i < kMaxMessageProcessedPerStep; i++) {
                auto message = websocket->pollMessages();
                if (!message.has_value())
                    break;

                CHARGELAB_TRY {
                    onMessage(*websocket, message.value());
                } CHARGELAB_CATCH {
                    CHARGELAB_LOG_MESSAGE(debug) << "Unexpected failed processing message: " << e.what();
                    dispatchUnexpectedMessage(message.value());
                }
            }

            OcppRemote remote {
                    *websocket,
                    registration_complete_,
                    [this](std::string const& unique_id, ActionId const& action) {
                        auto const now = system_->steadyClockNow();
                        if (last_call_.has_value()) {
                            auto const elapsed_seconds = (now - last_call_->timestamp)/1000;
                            if (elapsed_seconds < settings_->DefaultMessageTimeout.getValue()) {
                                CHARGELAB_LOG_MESSAGE(info) << "Call already in process - blocking call '" << action << "' waiting for: " << last_call_;
                                return false;
                            }

                            CHARGELAB_LOG_MESSAGE(info) << "Call already in process but timeout elapsed - allowing call '" << action << "' waiting for: " << last_call_;
                        }

                        last_call_ = detail::PendingCall {unique_id, action, now};
                        return true;
                    }
            };

            for (auto& x : services_)
                x->runStep(remote);
        }

        template <typename T, typename... Args>
        void addAfter(std::shared_ptr<T> module, Args&&... args) {
            auto index = getMaxIndex(std::forward<Args>(args)...);
            if (index.has_value()) {
                modules_.insert(modules_.begin()+index.value()+1, std::move(module));
            } else {
                modules_.push_back(std::move(module));
            }

            updatePointers();
        }

        template <typename T, typename... Args>
        void addBefore(std::shared_ptr<T> module, Args&&... args) {
            auto index = getMinIndex(std::forward<Args>(args)...);
            if (index.has_value()) {
                modules_.insert(modules_.begin()+index.value(), std::move(module));
            } else {
                modules_.push_back(std::move(module));
            }

            updatePointers();
        }

    private:
        void updatePointers() {
            services_.clear();
            request_handlers_.clear();
            response_handlers_.clear();
            pure_services_.clear();

            for (auto const& x : modules_) {
                if (x != nullptr) {
                    auto implementations = x->getImplementations();
                    addIfNotNull(services_, implementations.ocpp2_0.service);
                    addIfNotNull(request_handlers_, implementations.ocpp2_0.request_handler);
                    addIfNotNull(response_handlers_, implementations.ocpp2_0.response_handler);
                    addIfNotNull(pure_services_, implementations.pure_service);
                }
            }
        }

        std::optional<std::size_t> getMinIndex() {
            return std::nullopt;
        }

        template <typename Head, typename... Args>
        std::optional<std::size_t> getMinIndex(Head&& head, Args&&... args) {
            auto const next = getMinIndex(std::forward<Args>(args)...);
            for (std::size_t i=0; i < modules_.size(); i++) {
                if (head == modules_[i]) {
                    if (next.has_value()) {
                        return std::min(i, next.value());
                    } else {
                        return i;
                    }
                }
            }

            return next;
        }

        std::optional<std::size_t> getMaxIndex() {
            return std::nullopt;
        }

        template <typename Head, typename... Args>
        std::optional<std::size_t> getMaxIndex(Head&& head, Args&&... args) {
            auto const next = getMaxIndex(std::forward<Args>(args)...);
            for (std::size_t i=0; i < modules_.size(); i++) {
                if (head == modules_[i]) {
                    if (next.has_value()) {
                        return std::max(i, next.value());
                    } else {
                        return i;
                    }
                }
            }

            return next;
        }

    private:
        void onMessage(WebsocketInterface& websocket, std::string const& message) {
            CHARGELAB_LOG_MESSAGE(debug) << "Received OCPP message: " << message;

            stream::StringReader stream {message};
            json::JsonReader reader {stream};
            if (!json::expect_type<json::StartArrayType>(reader)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - expected array: " << message;
                dispatchUnexpectedMessage(message);
                return;
            }

            MessageType message_type;
            if (!json::ReadValue<MessageType>::read_json(reader, message_type)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad message type: " << message;
                dispatchUnexpectedMessage(message);
                return;
            }

            switch (message_type) {
                case MessageType::kValueNotFoundInEnum:
                    dispatchUnexpectedMessage(message);
                    break;

                case MessageType::kCall:
                    {
                        std::string unique_id;
                        if (!json::ReadValue<std::string>::read_json(reader, unique_id)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad unique ID: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        ActionId action_id;
                        if (!json::ReadValue<ActionId>::read_json(reader, action_id)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad action ID: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        switch (action_id) {
                            default:
                                CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - bad action ID: " << message;
                                dispatchUnexpectedMessage(message);
                                return;

#define CHARGELAB_ON_CALL_IMPL(TYPE)                                                                                                           \
                            case ActionId::k ## TYPE:                                                                                          \
                            {                                                                                                                  \
                                TYPE ## Request parsed {};                                                                                     \
                                if (!json::ReadValue<TYPE ## Request>::read_json(reader, parsed)) {                                            \
                                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - failed deserializing message payload: " << message;    \
                                    dispatchUnexpectedMessage(message);                                                                        \
                                    break;                                                                                                     \
                                }                                                                                                              \
                                                                                                                                               \
                                dispatchCall(                                                                                                  \
                                        websocket,                                                                                             \
                                        action_id,                                                                                             \
                                        unique_id,                                                                                             \
                                        common::RawJson::from_value_lazy(parsed),                                                              \
                                        [](                                                                                                    \
                                                AbstractRequestHandler* handler,                                                               \
                                                bool first,                                                                                    \
                                                TYPE ## Request const& req,                                                                    \
                                                WebsocketInterface& websocket,                                                                 \
                                                std::string const& unique_id                                                                   \
                                        ) {                                                                                                    \
                                            auto const response = handler->on ## TYPE ## Req(req);                                             \
                                            if (response.has_value()) {                                                                        \
                                                if (first) {                                                                                   \
                                                    sendResponse(websocket, unique_id, response.value());                                      \
                                                }                                                                                              \
                                                                                                                                               \
                                                return true;                                                                                   \
                                            } else {                                                                                           \
                                                return false;                                                                                  \
                                            }                                                                                                  \
                                        },                                                                                                     \
                                        parsed,                                                                                                \
                                        websocket,                                                                                             \
                                        unique_id                                                                                              \
                                );                                                                                                             \
                                break;                                                                                                         \
                            }
                            CHARGELAB_PASTE(CHARGELAB_ON_CALL_IMPL, CHARGELAB_OCPP_2_0_ACTION_IDS)
#undef CHARGELAB_ON_CALL_IMPL
                        }

                        if (!json::expect_type<json::EndArrayType>(reader))
                            CHARGELAB_LOG_MESSAGE(warning) << "Unexpected elements in OCPP call: " << message;

                        return;
                    }

                case MessageType::kCallResult:
                    {
                        std::string unique_id;
                        if (!json::ReadValue<std::string>::read_json(reader, unique_id)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad unique ID: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        std::optional<ActionId> action_id;
                        if (last_call_.has_value() && string::EqualsIgnoreCaseAscii(last_call_->unique_id, unique_id)) {
                            action_id = last_call_->action_id;
                            last_call_ = std::nullopt;
                        }

                        if (!action_id.has_value()) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Response ID not found in pending call list - treating as unexpected message: " << unique_id;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        switch (action_id.value()) {
                            default:
                                CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - bad action ID: " << message;
                                dispatchUnexpectedMessage(message);
                                return;

#define CHARGELAB_ON_CALL_RESPONSE_IMPL(TYPE)                                                                                               \
                            case ActionId::k ## TYPE:                                                                                       \
                            {                                                                                                               \
                                TYPE ## Response parsed {};                                                                                 \
                                if (!json::ReadValue<TYPE ## Response>::read_json(reader, parsed)) {                                        \
                                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - failed deserializing message payload: " << message; \
                                    dispatchUnexpectedMessage(message);                                                                     \
                                    return;                                                                                                 \
                                }                                                                                                           \
                                                                                                                                            \
                                for (auto& ptr : response_handlers_) {                                                                      \
                                    if (ptr != nullptr) {                                                                                   \
                                        ptr->on ## TYPE ## Rsp(unique_id, parsed);                                                          \
                                        ptr->onCallRsp(unique_id, common::RawJson::from_value_lazy(parsed));                                \
                                    }                                                                                                       \
                                }                                                                                                           \
                                break;                                                                                                      \
                            }
                            CHARGELAB_PASTE(CHARGELAB_ON_CALL_RESPONSE_IMPL, CHARGELAB_OCPP_2_0_ACTION_IDS)
#undef CHARGELAB_ON_CALL_RESPONSE_IMPL
                        }

                        // Note: ignoring and allowing
                        if (!json::expect_type<json::EndArrayType>(reader))
                            CHARGELAB_LOG_MESSAGE(warning) << "Unexpected elements in OCPP call: " << message;

                        return;
                    }

                case MessageType::kCallError:
                    {
                        std::string unique_id;
                        if (!json::ReadValue<std::string>::read_json(reader, unique_id)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad unique ID: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        ErrorCode error_code;
                        if (!json::ReadValue<ErrorCode>::read_json(reader, error_code)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad error code: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        std::string description;
                        if (!json::ReadValue<std::string>::read_json(reader, description)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad description: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        common::RawJson details;
                        if (!json::ReadValue<common::RawJson>::read_json(reader, details)) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - missing or bad details: " << message;
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        // Note: ignoring and allowing
                        if (!json::expect_type<json::EndArrayType>(reader))
                            CHARGELAB_LOG_MESSAGE(warning) << "Unexpected elements in OCPP call: " << message;

                        auto const error = CallError {error_code, description, details};
                        CHARGELAB_LOG_MESSAGE(debug) << "Received OCPP error: id=" << unique_id << ", error=" << error;

                        std::optional<ActionId> action_id;
                        if (last_call_.has_value() && string::EqualsIgnoreCaseAscii(last_call_->unique_id, unique_id)) {
                            action_id = last_call_->action_id;
                            last_call_ = std::nullopt;
                        }

                        if (!action_id.has_value()) {
                            dispatchUnexpectedMessage(message);
                            return;
                        }

                        switch (action_id.value()) {
                            default:
                                CHARGELAB_LOG_MESSAGE(warning) << "Bad message payload - bad action ID: " << message;
                                dispatchUnexpectedMessage(message);
                                return;

#define CHARGELAB_ON_CALL_ERROR_IMPL(TYPE)                                                                                                  \
                            case ActionId::k ## TYPE:                                                                                       \
                            {                                                                                                               \
                                for (auto& ptr : response_handlers_) {                                                                      \
                                    if (ptr != nullptr) {                                                                                   \
                                        ptr->on ## TYPE ## Rsp(unique_id, error);                                                           \
                                    }                                                                                                       \
                                }                                                                                                           \
                                break;                                                                                                      \
                            }
                            CHARGELAB_PASTE(CHARGELAB_ON_CALL_ERROR_IMPL, CHARGELAB_OCPP_2_0_ACTION_IDS)
#undef CHARGELAB_ON_CALL_ERROR_IMPL
                        }

                        for (auto& ptr : response_handlers_) {
                            if (ptr != nullptr) {
                                ptr->onCallRsp(unique_id, error);
                            }
                        }

                        return;
                    }
            }
        }

        bool allowFallthrough(ActionId const& action_id) {
            switch (action_id) {
                default:
                    return false;

//                case ActionId::kTriggerMessage:
//                case ActionId::kChangeConfiguration:
//                    return true;
            }
        }

        void dispatchUnexpectedMessage(std::string const& message) {
            CHARGELAB_LOG_MESSAGE(debug) << "Dispatching unexpected message: " << message;

            for (auto& ptr : services_) {
                if (ptr != nullptr)
                    ptr->onUnmanagedMessage(message);
            }
        }

        template<typename T>
        static void sendResponse(WebsocketInterface& websocket, std::string const& unique_id, ResponseToRequest<T> const& result) {
            if (std::holds_alternative<T>(result)) {
                websocket.sendCustom([&](ByteWriterInterface& stream) {
                    json::JsonWriter writer {stream};
                    writer.StartArray();
                    writer.Int((int)MessageType::kCallResult);
                    writer.String(unique_id);
                    json::WriteValue<T>::write_json(writer, std::get<T>(result));
                    writer.EndArray();
                });
            } else if (std::holds_alternative<CallError>(result)) {
                sendError(websocket, unique_id, std::get<CallError>(result));
            } else {
                std::get<CustomResponse>(result)(websocket, unique_id);
            }
        }

        static void sendError(WebsocketInterface& websocket, std::string const& unique_id, CallError const& error) {
            websocket.sendCustom([&](ByteWriterInterface& stream) {
                json::JsonWriter writer {stream};
                writer.StartArray();
                writer.Int((int)MessageType::kCallError);
                writer.String(unique_id);
                json::WriteValue<ErrorCode>::write_json(writer, error.code);
                writer.String(error.description.value());
                json::WriteValue<common::RawJson>::write_json(writer, error.details);
                writer.EndArray();
            });
        }


        template <typename F, typename... Args>
        void dispatchCall(
                WebsocketInterface& websocket,
                ActionId const& action_id,
                std::string const& unique_id,
                common::RawJson const& payload,
                F&& function,
                Args&&... args
        ) {
            bool responded = false;
            for (auto& ptr : request_handlers_) {
                if (ptr != nullptr) {
                    if (!responded) {
                        responded = function(ptr, true, std::forward<Args>(args)...);
                    } else if (allowFallthrough(action_id)) {
                        function(ptr, false, std::forward<Args>(args)...);
                    }

                    if (!responded) {
                        auto response = ptr->onCall(action_id, payload);
                        if (response.has_value()) {
                            sendResponse(websocket, unique_id, response.value());
                            responded = true;
                        }
                    } else if (allowFallthrough(action_id)) {
                        ptr->onCall(action_id, payload);
                    }
                }
            }

            if (!responded) {
                sendError(websocket, unique_id, CallError {
                        ErrorCode::kNotImplemented,
                        "Not implemented",
                        common::RawJson::empty_object()
                });
            }
        }

        template <typename T>
        static void addIfNotNull(std::vector<T*>& container, T* x) {
            if (x != nullptr) {
                container.push_back(x);
            }
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_;
        std::vector<std::shared_ptr<AbstractModuleInterface>> modules_;
        std::function<bool()> registration_complete_;

        std::vector<std::shared_ptr<void>> wrappers_;
        std::vector<AbstractService*> services_;
        std::vector<AbstractRequestHandler*> request_handlers_;
        std::vector<AbstractResponseHandler*> response_handlers_;
        std::vector<chargelab::detail::PureServiceInterface*> pure_services_;

        std::optional<detail::PendingCall> last_call_ = std::nullopt;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_MESSAGE_HANDLER_H
