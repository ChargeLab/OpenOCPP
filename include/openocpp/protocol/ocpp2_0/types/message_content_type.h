#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_CONTENT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_CONTENT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/message_format_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct MessageContentType {
        MessageFormatEnumType format{};
        std::optional<StringPrimitive<8>> language{};
        StringPrimitive<512> content{};
        CHARGELAB_JSON_INTRUSIVE(MessageContentType, format, language, content)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_CONTENT_TYPE_H
