#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_SERVICE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_SERVICE_H

#include "openocpp/protocol/ocpp1_6/handlers/ocpp_remote.h"
#include "openocpp/protocol/common/raw_json.h"

namespace chargelab::ocpp1_6 {
    class AbstractService {
    public:
        virtual void runStep(OcppRemote& remote) {
            (void)remote;
        };

        virtual void onUnmanagedMessage(std::string const& message) {
            (void)message;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ABSTRACT_SERVICE_H
