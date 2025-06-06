#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CONNECTOR_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CONNECTOR_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    class ConnectorEnumType {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kcCCS1,
            kcCCS2,
            kcG105,
            kcTesla,
            kcType1,
            kcType2,
            ks309_1P_16A,
            ks309_1P_32A,
            ks309_3P_16A,
            ks309_3P_32A,
            ksBS1361,
            ksCEE_7_7,
            ksType2,
            ksType3,
            kOther1PhMax16A,
            kOther1PhOver16A,
            kOther3Ph,
            kPan,
            kwInductive,
            kwResonant,
            kUndetermined,
            kUnknown
        };

        CHARGELAB_JSON_ENUM_CUSTOM(ConnectorEnumType, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kcCCS1, "cCCS1"},
            {kcCCS2, "cCCS2"},
            {kcG105, "cG105"},
            {kcTesla, "cTesla"},
            {kcType1, "cType1"},
            {kcType2, "cType2"},
            {ks309_1P_16A, "s309-1P-16A"},
            {ks309_1P_32A, "s309-1P-32A"},
            {ks309_3P_16A, "s309-3P-16A"},
            {ks309_3P_32A, "s309-3P-32A"},
            {ksBS1361, "sBS1361"},
            {ksCEE_7_7, "sCEE-7-7"},
            {ksType2, "sType2"},
            {ksType3, "sType3"},
            {kOther1PhMax16A, "Other1PhMax16A"},
            {kOther1PhOver16A, "Other1PhOver16A"},
            {kOther3Ph, "Other3Ph"},
            {kPan, "Pan"},
            {kwInductive, "wInductive"},
            {kwResonant, "wResonant"},
            {kUndetermined, "Undetermined"},
            {kUnknown, "Unknown"}
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CONNECTOR_ENUM_TYPE_H
