#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CALL_RESULT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CALL_RESULT_H

#include "openocpp/protocol/ocpp2_0/types/call_error.h"
#include "openocpp/interface/element/websocket_interface.h"

#include <optional>
#include <variant>

namespace chargelab::ocpp2_0 {
    using CustomResponse = std::function<void(WebsocketInterface&, std::string const&)>;

    template<typename T>
    using ResponseMessage = std::variant<T, CallError>;

    template<typename T>
    using ResponseToRequest = std::variant<T, CallError, CustomResponse>;
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CALL_RESULT_H
