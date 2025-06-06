#ifndef CHARGELAB_OPEN_FIRMWARE_WEBSOCKET_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_WEBSOCKET_INTERFACE_H

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

#include "openocpp/helpers/json.h"
#include "openocpp/interface/element/byte_writer_interface.h"

namespace chargelab {
    class WebsocketInterface {
    public:
        virtual ~WebsocketInterface() = default;

        virtual bool isConnected() = 0;
        virtual std::optional<std::string> getSubprotocol() = 0;

        virtual std::size_t pendingMessages() = 0;
        virtual std::optional<std::string> pollMessages() = 0;
        virtual void sendCustom(std::function<void(ByteWriterInterface&)> payload) = 0;

        template <typename T>
        void sendJson(T const& value) {
            sendCustom([&](ByteWriterInterface& stream) {
                write_json(stream, value);
            });
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_WEBSOCKET_INTERFACE_H
