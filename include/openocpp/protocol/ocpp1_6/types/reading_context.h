#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_READING_CONTEXT_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_READING_CONTEXT_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class ReadingContext {
    public:
        enum Value {
            kValueNotFoundInEnum,
            kInterruptionBegin,
            kInterruptionEnd,
            kOther,
            kSampleClock,
            kSamplePeriodic,
            kTransactionBegin,
            kTransactionEnd,
            kTrigger
        };

        CHARGELAB_JSON_ENUM_CUSTOM(ReadingContext, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kInterruptionBegin, "Interruption.Begin"},
            {kInterruptionEnd, "Interruption.End"},
            {kOther, "Other"},
            {kSampleClock, "Sample.Clock"},
            {kSamplePeriodic, "Sample.Periodic"},
            {kTransactionBegin, "Transaction.Begin"},
            {kTransactionEnd, "Transaction.End"},
            {kTrigger, "Trigger"},
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_READING_CONTEXT_H
