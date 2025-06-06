#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_READING_CONTEXT_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_READING_CONTEXT_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    class ReadingContextEnumType{
    public:
        enum Value {
            kValueNotFoundInEnum,
            kInterruption_Begin,
            kInterruption_End,
            kOther,
            kSample_Clock,
            kSample_Periodic,
            kTransaction_Begin,
            kTransaction_End,
            kTrigger
        };

        CHARGELAB_JSON_ENUM_CUSTOM(ReadingContextEnumType, {
            {kValueNotFoundInEnum, "ValueNotFoundInEnum"},
            {kInterruption_Begin, "Interruption.Begin"},
            {kInterruption_End, "Interruption.End"},
            {kOther, "Other"},
            {kSample_Clock, "Sample.Clock"},
            {kSample_Periodic, "Sample.Periodic"},
            {kTransaction_Begin, "Transaction.Begin"},
            {kTransaction_End, "Transaction.End"},
            {kTrigger, "Trigger"}
        })
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_READING_CONTEXT_ENUM_TYPE_H
