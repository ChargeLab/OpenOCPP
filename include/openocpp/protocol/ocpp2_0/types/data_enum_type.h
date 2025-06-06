#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_DATA_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_DATA_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(DataEnumType, 
        string,
        decimal,
        integer,
        dateTime,
        boolean,
        OptionList,
        SequenceList,
        MemberList
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_DATA_ENUM_TYPE_H
