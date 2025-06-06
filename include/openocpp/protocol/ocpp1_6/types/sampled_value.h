#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_SAMPLED_VALUE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_SAMPLED_VALUE_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/reading_context.h"
#include "openocpp/protocol/ocpp1_6/types/value_format.h"
#include "openocpp/protocol/ocpp1_6/types/measurand.h"
#include "openocpp/protocol/ocpp1_6/types/phase.h"
#include "openocpp/protocol/ocpp1_6/types/location.h"
#include "openocpp/protocol/ocpp1_6/types/unit_of_measure.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct SampledValue {
        std::string value;
        std::optional<ReadingContext> context;
        std::optional<ValueFormat> format;
        std::optional<Measurand> measurand;
        std::optional<Phase> phase;
        std::optional<Location> location;
        std::optional<UnitOfMeasure> unit;
        CHARGELAB_JSON_INTRUSIVE(SampledValue, value, context, format, measurand, phase, location, unit)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_SAMPLED_VALUE_H
