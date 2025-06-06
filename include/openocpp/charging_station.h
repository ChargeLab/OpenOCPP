#ifndef CHARGELAB_OPEN_FIRMWARE_CHARGING_STATION_H
#define CHARGELAB_OPEN_FIRMWARE_CHARGING_STATION_H

#include "openocpp/interface/element/websocket_raw_interface.h"
#include "openocpp/interface/message_handler.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/element/storage_interface.h"
#include "openocpp/interface/connector_interface.h"
#include "openocpp/helpers/optional.h"
#include "openocpp/helpers/string.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/logging.h"

#include <memory>
#include <utility>

namespace chargelab {
    class ChargingStation {
    public:
        ChargingStation(std::shared_ptr<WebsocketInterface> websocket, std::shared_ptr<MessageHandlerInterface> handler)
            : websocket_(std::move(websocket)), handler_(std::move(handler))
        {
        }

        ~ChargingStation() {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting ChargingStation";
        }

    public:
        void runStep() {
            auto connected = websocket_->isConnected();
            if (connected) {
                optional::IfPresent(websocket_->pollTextMessages(), [&](auto&& x) { handler_->onTextMessage(x); });
                optional::IfPresent(websocket_->pollBinaryMessages(), [&](auto&& x) { handler_->onBinaryMessage(x); });
            }
            handler_->runStep();
            if (connected) {
                optional::IfPresent(handler_->pollTextMessages(), [&](auto&& x) { websocket_->sendTextMessage(x); });
                optional::IfPresent(handler_->pollBinaryMessages(), [&](auto&& x) { websocket_->sendBinaryMessage(x); });
            }
        }

    private:
        std::shared_ptr<SystemInterface> system_;
        std::shared_ptr<WebsocketInterface> websocket_;
        std::shared_ptr<MessageHandlerInterface> handler_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_CHARGING_STATION_H
