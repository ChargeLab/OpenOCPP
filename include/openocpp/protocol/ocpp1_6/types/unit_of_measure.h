#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_UNIT_OF_MEASURE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_UNIT_OF_MEASURE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class UnitOfMeasure {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kWattHours,
            kKiloWattHours,
            kVarHours,
            kKiloVarHours,
            kWatts,
            kKiloWatts,
            kVoltAmps,
            kKiloVoltAmps,
            kVars,
            kKiloVars,
            kAmps,
            kVolts,
            kCelsius,
            kFahrenheit,
            kKelvin,
            kPercent
        };

        CHARGELAB_JSON_ENUM_CUSTOM(UnitOfMeasure, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kWattHours, "Wh"},
            {kKiloWattHours, "kWh"},
            {kVarHours, "varh"},
            {kKiloVarHours, "kvarh"},
            {kWatts, "W"},
            {kKiloWatts, "kW"},
            {kVoltAmps, "VA"},
            {kKiloVoltAmps, "kVA"},
            {kVars, "var"},
            {kKiloVars, "kvar"},
            {kAmps, "A"},
            {kVolts, "V"},
            {kCelsius, "Celsius"},
            {kFahrenheit, "Fahrenheit"},
            {kKelvin, "K"},
            {kPercent, "Percent"}
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_UNIT_OF_MEASURE_H
