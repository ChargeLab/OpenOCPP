#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_SERVICE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_SERVICE_H

#include "openocpp/protocol/ocpp2_0/handlers/ocpp_remote.h"

namespace chargelab::ocpp2_0 {
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

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ABSTRACT_SERVICE_H
