#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_attribute_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_characteristics_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ReportDataType {
        ComponentType component {};
        VariableType variable {};
        std::vector<VariableAttributeType> variableAttribute {};
        std::optional<VariableCharacteristicsType> variableCharacteristics {};
        CHARGELAB_JSON_INTRUSIVE(ReportDataType, component, variable, variableAttribute, variableCharacteristics)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_DATA_TYPE_H
