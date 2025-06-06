#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_MEASURAND_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_MEASURAND_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class Measurand {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kCurrentExport,
            kCurrentImport,
            kCurrentOffered,
            kEnergyActiveExportRegister,
            kEnergyActiveImportRegister,
            kEnergyReactiveExportRegister,
            kEnergyReactiveImportRegister,
            kEnergyActiveExportInterval,
            kEnergyActiveImportInterval,
            kEnergyReactiveExportInterval,
            kEnergyReactiveImportInterval,
            kFrequency,
            kPowerActiveExport,
            kPowerActiveImport,
            kPowerFactor,
            kPowerOffered,
            kPowerReactiveExport,
            kPowerReactiveImport,
            kRPM,
            kSoC,
            kTemperature,
            kVoltage
        };

        CHARGELAB_JSON_ENUM_CUSTOM(Measurand, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kCurrentExport, "Current.Export"},
            {kCurrentImport, "Current.Import"},
            {kCurrentOffered, "Current.Offered"},
            {kEnergyActiveExportRegister, "Energy.Active.Export.Register"},
            {kEnergyActiveImportRegister, "Energy.Active.Import.Register"},
            {kEnergyReactiveExportRegister, "Energy.Reactive.Export.Register"},
            {kEnergyReactiveImportRegister, "Energy.Reactive.Import.Register"},
            {kEnergyActiveExportInterval, "Energy.Active.Export.Interval"},
            {kEnergyActiveImportInterval, "Energy.Active.Import.Interval"},
            {kEnergyReactiveExportInterval, "Energy.Reactive.Export.Interval"},
            {kEnergyReactiveImportInterval, "Energy.Reactive.Import.Interval"},
            {kFrequency, "Frequency"},
            {kPowerActiveExport, "Power.Active.Export"},
            {kPowerActiveImport, "Power.Active.Import"},
            {kPowerFactor, "Power.Factor"},
            {kPowerOffered, "Power.Offered"},
            {kPowerReactiveExport, "Power.Reactive.Export"},
            {kPowerReactiveImport, "Power.Reactive.Import"},
            {kRPM, "RPM"},
            {kSoC, "SoC"},
            {kTemperature, "Temperature"},
            {kVoltage, "Voltage"},
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_MEASURAND_H
