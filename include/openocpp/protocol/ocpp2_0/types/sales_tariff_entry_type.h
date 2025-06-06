#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_ENTRY_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_ENTRY_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SalesTariffEntryType {
        std::optional<int> ePriceLevel {};
        int relativeTimeInterval {};
        int consumptionCost {};
        CHARGELAB_JSON_INTRUSIVE(SalesTariffEntryType, ePriceLevel, relativeTimeInterval, consumptionCost)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_ENTRY_TYPE_H
