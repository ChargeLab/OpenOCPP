#ifndef CHARGELAB_OPEN_FIRMWARE_TX_START_POINT_VALUES_H
#define CHARGELAB_OPEN_FIRMWARE_TX_START_POINT_VALUES_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(TxStartPointValues, 
        ParkingBayOccupancy,
        EVConnected,
        Authorized,
        PowerPathClosed,
        EnergyTransfer,
        DataSigned
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_TX_START_POINT_VALUES_H
