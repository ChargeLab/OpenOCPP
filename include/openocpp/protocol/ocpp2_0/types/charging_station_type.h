#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATION_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATION_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/modem_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingStationType {
        std::optional<StringPrimitive<25>> serialNumber {};
        StringPrimitive<20> model {};
        StringPrimitive<50> vendorName {};
        std::optional<StringPrimitive<50>> firmwareVersion {};
        std::optional<ModemType> modem {};
        CHARGELAB_JSON_INTRUSIVE(ChargingStationType, serialNumber, model, vendorName, firmwareVersion, modem)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATION_TYPE_H
