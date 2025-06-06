#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ID_TAG_INFO_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ID_TAG_INFO_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/authorization_status.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct IdTagInfo {
        std::optional<DateTime> expiryDate;
        std::optional<CiString20Type> parentIdTag;
        AuthorizationStatus status;
        CHARGELAB_JSON_INTRUSIVE(IdTagInfo, expiryDate, parentIdTag, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ID_TAG_INFO_H
