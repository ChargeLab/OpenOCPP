#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MEASURAND_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MEASURAND_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    class MeasurandEnumType {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kCurrent_Export,
            kCurrent_Import,
            kCurrent_Offered,
            kEnergy_Active_Export_Register,
            kEnergy_Active_Import_Register,
            kEnergy_Reactive_Export_Register,
            kEnergy_Reactive_Import_Register,
            kEnergy_Active_Export_Interval,
            kEnergy_Active_Import_Interval,
            kEnergy_Active_Net,
            kEnergy_Reactive_Export_Interval,
            kEnergy_Reactive_Import_Interval,
            kEnergy_Reactive_Net,
            kEnergy_Apparent_Net,
            kEnergy_Apparent_Import,
            kEnergy_Apparent_Export,
            kFrequency,
            kPower_Active_Export,
            kPower_Active_Import,
            kPower_Factor,
            kPower_Offered,
            kPower_Reactive_Export,
            kPower_Reactive_Import,
            kSoC,
            kVoltage
        };

        CHARGELAB_JSON_ENUM_CUSTOM(MeasurandEnumType, {
            {MeasurandEnumType::kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {MeasurandEnumType::kCurrent_Export, "Current.Export"},
            {MeasurandEnumType::kCurrent_Import, "Current.Import"},
            {MeasurandEnumType::kCurrent_Offered, "Current.Offered"},
            {MeasurandEnumType::kEnergy_Active_Export_Register, "Energy.Active.Export.Register"},
            {MeasurandEnumType::kEnergy_Active_Import_Register, "Energy.Active.Import.Register"},
            {MeasurandEnumType::kEnergy_Reactive_Export_Register, "Energy.Reactive.Export.Register"},
            {MeasurandEnumType::kEnergy_Reactive_Import_Register, "Energy.Reactive.Import.Register"},
            {MeasurandEnumType::kEnergy_Active_Export_Interval, "Energy.Active.Export.Interval"},
            {MeasurandEnumType::kEnergy_Active_Import_Interval, "Energy.Active.Import.Interval"},
            {MeasurandEnumType::kEnergy_Active_Net, "Energy.Active.Net"},
            {MeasurandEnumType::kEnergy_Reactive_Export_Interval, "Energy.Reactive.Export.Interval"},
            {MeasurandEnumType::kEnergy_Reactive_Import_Interval, "Energy.Reactive.Import.Interval"},
            {MeasurandEnumType::kEnergy_Reactive_Net, "Energy.Reactive.Net"},
            {MeasurandEnumType::kEnergy_Apparent_Net, "Energy.Apparent.Net"},
            {MeasurandEnumType::kEnergy_Apparent_Import, "Energy.Apparent.Import"},
            {MeasurandEnumType::kEnergy_Apparent_Export, "Energy.Apparent.Export"},
            {MeasurandEnumType::kFrequency, "Frequency"},
            {MeasurandEnumType::kPower_Active_Export, "Power.Active.Export"},
            {MeasurandEnumType::kPower_Active_Import, "Power.Active.Import"},
            {MeasurandEnumType::kPower_Factor, "Power.Factor"},
            {MeasurandEnumType::kPower_Offered, "Power.Offered"},
            {MeasurandEnumType::kPower_Reactive_Export, "Power.Reactive.Export"},
            {MeasurandEnumType::kPower_Reactive_Import, "Power.Reactive.Import"},
            {MeasurandEnumType::kSoC, "SoC"},
            {MeasurandEnumType::kVoltage, "Voltage"}
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MEASURAND_ENUM_TYPE_H
