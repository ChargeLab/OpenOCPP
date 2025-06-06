#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CUSTOMER_INFORMATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CUSTOMER_INFORMATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyCustomerInformationRequest {
        StringPrimitive<512> data {};
        std::optional<bool> tbc {};
        int seqNo {};
        DateTime generatedAt {};
        int requestId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyCustomerInformationRequest, kNotifyCustomerInformation, data, tbc, seqNo, generatedAt, requestId)
    };


    struct NotifyCustomerInformationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyCustomerInformationResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CUSTOMER_INFORMATION_H
