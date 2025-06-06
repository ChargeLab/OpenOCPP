#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SAMPLED_VALUE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SAMPLED_VALUE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/reading_context_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/measurand_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/phase_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/location_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/signed_meter_value_type.h"
#include "openocpp/protocol/ocpp2_0/types/unit_of_measure_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SampledValueType {
        double value {};
        std::optional<ReadingContextEnumType> context {};
        std::optional<MeasurandEnumType> measurand {};
        std::optional<PhaseEnumType> phase {};
        std::optional<LocationEnumType> location {};
        std::optional<SignedMeterValueType> signedMeterValue {};
        std::optional<UnitOfMeasureType> unitOfMeasure {};
        CHARGELAB_JSON_INTRUSIVE(SampledValueType, value, context, measurand, phase, location, signedMeterValue, unitOfMeasure)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SAMPLED_VALUE_TYPE_H
