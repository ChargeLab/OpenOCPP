#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_PHASE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_PHASE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class Phase {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kOnL1,
            kOnL2,
            kOnL3,
            kOnNeutral,
            kOnL1RelativeToNeutral,
            kOnL2RelativeToNeutral,
            kOnL3RelativeToNeutral,
            kBetweenL1AndL2,
            kBetweenL2AndL3,
            kBetweenL3AndL1
        };

        CHARGELAB_JSON_ENUM_CUSTOM(Phase, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kOnL1, "L1"},
            {kOnL2, "L2"},
            {kOnL3, "L3"},
            {kOnNeutral, "N"},
            {kOnL1RelativeToNeutral, "L1-N"},
            {kOnL2RelativeToNeutral, "L2-N"},
            {kOnL3RelativeToNeutral, "L3-N"},
            {kBetweenL1AndL2, "L1-L2"},
            {kBetweenL2AndL3, "L2-L3"},
            {kBetweenL3AndL1, "L3-L1"},
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_PHASE_H
