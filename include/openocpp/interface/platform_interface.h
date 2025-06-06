#ifndef CHARGELAB_OPEN_FIRMWARE_PLATFORM_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_PLATFORM_INTERFACE_H

#include "openocpp/model/system_types.h"
#include "openocpp/interface/element/rest_connection_interface.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/component/upload_interface.h"
#include "openocpp/interface/element/websocket_interface.h"
#include "openocpp/common/settings.h"
#include "openocpp/module/common_templates.h"

#include <cstdint>
#include <memory>

namespace chargelab {
    enum class RestMethod {
        kGet,
        kPost,
        kPut,
        kPatch,
        kDelete,
        kHead,
        kOptions
    };

    struct SignatureAndHash {
        const unsigned char *hash = nullptr;
        std::size_t hash_len = 0;

        const unsigned char *sig = nullptr;
        std::size_t sig_len = 0;
    };

    class PlatformInterface :
            public SystemInterface,
            public ServiceStatefulGeneral
    {
    public:
        virtual std::shared_ptr<Settings> getSettings() = 0;
        virtual std::shared_ptr<WebsocketInterface> ocppConnection() = 0;
        virtual std::shared_ptr<RestConnectionInterface> restRequest(RestMethod method, std::string const& uri) = 0;

        virtual bool verifyManufacturerCertificate(
                std::string const& pem,
                std::optional<SignatureAndHash> const& check_sha256
        ) = 0;

        virtual std::shared_ptr<RestConnectionInterface> getRequest(std::string const& uri) {
            return restRequest(RestMethod::kGet, uri);
        }

        virtual std::shared_ptr<RestConnectionInterface> putRequest(std::string const& uri) {
            return restRequest(RestMethod::kPut, uri);
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_PLATFORM_INTERFACE_H
