#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_PHASE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_PHASE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    class PhaseEnumType {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kL1,
            kL2,
            kL3,
            kN,
            kL1_N,
            kL2_N,
            kL3_N,
            kL1_L2,
            kL2_L3,
            kL3_L1
        };

        CHARGELAB_JSON_ENUM_CUSTOM(PhaseEnumType, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kL1, "L1"},
            {kL2, "L2"},
            {kL3, "L3"},
            {kN, "N"},
            {kL1_N, "L1-N"},
            {kL2_N, "L2-N"},
            {kL3_N, "L3-N"},
            {kL1_L2, "L1-L2"},
            {kL2_L3, "L2-L3"},
            {kL3_L1, "L3-L1"}
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_PHASE_ENUM_TYPE_H
