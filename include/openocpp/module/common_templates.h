#ifndef CHARGELAB_OPEN_FIRMWARE_COMMON_TEMPLATES_H
#define CHARGELAB_OPEN_FIRMWARE_COMMON_TEMPLATES_H

#include "openocpp/protocol/ocpp1_6/handlers/abstract_request_handler.h"
#include "openocpp/protocol/ocpp1_6/handlers/abstract_response_handler.h"
#include "openocpp/protocol/ocpp1_6/handlers/abstract_service.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_request_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_response_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_service.h"
#include "openocpp/module/abstract_module.h"

namespace chargelab {
    class ServiceStateless1_6 :
            public AbstractModuleBase<ServiceStateless1_6>,
            private detail::PureServiceInterface,
            private ocpp1_6::AbstractRequestHandler
    {
        friend class AbstractModuleBase<ServiceStateless1_6>;
    };

    class ServiceStateful1_6 :
            public AbstractModuleBase<ServiceStateful1_6>,
            private detail::PureServiceInterface,
            private ocpp1_6::AbstractService,
            private ocpp1_6::AbstractRequestHandler,
            private ocpp1_6::AbstractResponseHandler
    {
        friend class AbstractModuleBase<ServiceStateful1_6>;
    };

    class ServiceStateless2_0 :
            public AbstractModuleBase<ServiceStateless2_0>,
            private detail::PureServiceInterface,
            private ocpp2_0::AbstractRequestHandler
    {
        friend class AbstractModuleBase<ServiceStateless2_0>;
    };

    class ServiceStateful2_0 :
            public AbstractModuleBase<ServiceStateful2_0>,
            private detail::PureServiceInterface,
            private ocpp2_0::AbstractService,
            private ocpp2_0::AbstractRequestHandler,
            private ocpp2_0::AbstractResponseHandler
    {
        friend class AbstractModuleBase<ServiceStateful2_0>;
    };

    class ServiceStatelessGeneral :
            public AbstractModuleBase<ServiceStatelessGeneral>,
            private detail::PureServiceInterface,
            private ocpp1_6::AbstractRequestHandler,
            private ocpp2_0::AbstractRequestHandler
    {
        friend class AbstractModuleBase<ServiceStatelessGeneral>;
    };

    class ServiceStatefulGeneral :
            public AbstractModuleBase<ServiceStatefulGeneral>,
            private detail::PureServiceInterface,
            private ocpp1_6::AbstractService,
            private ocpp1_6::AbstractRequestHandler,
            private ocpp1_6::AbstractResponseHandler,
            private ocpp2_0::AbstractService,
            private ocpp2_0::AbstractRequestHandler,
            private ocpp2_0::AbstractResponseHandler
    {
        friend class AbstractModuleBase<ServiceStatefulGeneral>;
    };

    class PureService :
            public AbstractModuleBase<PureService>,
            private detail::PureServiceInterface
    {
        friend class AbstractModuleBase<PureService>;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_COMMON_TEMPLATES_H
