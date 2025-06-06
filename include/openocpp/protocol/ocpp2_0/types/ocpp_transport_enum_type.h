#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_TRANSPORT_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_TRANSPORT_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(OCPPTransportEnumType, 
        JSON,
        SOAP
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_TRANSPORT_ENUM_TYPE_H
