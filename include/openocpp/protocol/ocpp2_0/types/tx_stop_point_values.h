#ifndef CHARGELAB_OPEN_FIRMWARE_TX_STOP_POINT_VALUES_H
#define CHARGELAB_OPEN_FIRMWARE_TX_STOP_POINT_VALUES_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    // Note: deviating from the specification by excluding this as a valid stop point. See 2.6.6.2. TxStopPoint
    // values; "DataSigned - This condition has no meaning as a TxStopPoint and should not be used as such."
    //,kDataSigned
    CHARGELAB_JSON_ENUM(TxStopPointValues, 
        ParkingBayOccupancy,
        EVConnected,
        Authorized,
        PowerPathClosed,
        EnergyTransfer
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_TX_STOP_POINT_VALUES_H
