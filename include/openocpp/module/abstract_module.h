#ifndef CHARGELAB_OPEN_FIRMWARE_ABSTRACT_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_ABSTRACT_MODULE_H

#include "openocpp/protocol/ocpp1_6/handlers/abstract_request_handler.h"
#include "openocpp/protocol/ocpp1_6/handlers/abstract_response_handler.h"
#include "openocpp/protocol/ocpp1_6/handlers/abstract_service.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_request_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_response_handler.h"
#include "openocpp/protocol/ocpp2_0/handlers/abstract_service.h"

namespace chargelab {
    namespace detail {
        class PureServiceInterface {
        public:
            virtual void runUnconditionally() {
            };
        };

        struct ConcreteImplementationsOCPP1_6 {
            ocpp1_6::AbstractService* service;
            ocpp1_6::AbstractRequestHandler* request_handler;
            ocpp1_6::AbstractResponseHandler* response_handler;
        };

        struct ConcreteImplementationsOCPP2_0 {
            ocpp2_0::AbstractService* service;
            ocpp2_0::AbstractRequestHandler* request_handler;
            ocpp2_0::AbstractResponseHandler* response_handler;
        };

        struct ConcreteImplementations {
            ConcreteImplementationsOCPP1_6 ocpp1_6;
            ConcreteImplementationsOCPP2_0 ocpp2_0;
            PureServiceInterface* pure_service;
        };
    }

    class AbstractModuleInterface {
    public:
        virtual ~AbstractModuleInterface() = default;
        virtual detail::ConcreteImplementations getImplementations() = 0;
    };

    template <typename T>
    class AbstractModuleBase : public AbstractModuleInterface {
    public:
        detail::ConcreteImplementations getImplementations() override {
            auto parent_this = static_cast<T*>(this);

            detail::ConcreteImplementations result {};
            PopulateImplementation(result.ocpp1_6.service, parent_this);
            PopulateImplementation(result.ocpp1_6.request_handler, parent_this);
            PopulateImplementation(result.ocpp1_6.response_handler, parent_this);
            PopulateImplementation(result.ocpp2_0.service, parent_this);
            PopulateImplementation(result.ocpp2_0.request_handler, parent_this);
            PopulateImplementation(result.ocpp2_0.response_handler, parent_this);
            PopulateImplementation(result.pure_service, parent_this);
            return result;
        }

    private:
        template <typename U>
        static typename std::enable_if<std::is_base_of<U, T>::value, U*>::type ConvertToBase(T* ptr) {
            return static_cast<U*>(ptr);
        }

        template <typename U>
        static typename std::enable_if<!std::is_base_of<U, T>::value, U*>::type ConvertToBase(T*) {
            return nullptr;
        }

        template <typename U>
        static void PopulateImplementation(U*& implementation, T* ptr) {
            implementation = ConvertToBase<U>(ptr);
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_ABSTRACT_MODULE_H
