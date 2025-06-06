#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/sales_tariff_entry_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SalesTariffType {
        int id {};
        std::optional<StringPrimitive<32>> salesTariffDescription {};
        std::optional<int> numEPriceLevels {};
        std::vector<SalesTariffEntryType> salesTariffEntry {};
        CHARGELAB_JSON_INTRUSIVE(SalesTariffType, id, salesTariffDescription, numEPriceLevels, salesTariffEntry)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SALES_TARIFF_TYPE_H
