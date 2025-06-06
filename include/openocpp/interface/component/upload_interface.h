#ifndef CHARGELAB_OPEN_FIRMWARE_UPLOAD_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_UPLOAD_INTERFACE_H

#include <string>
#include <functional>

namespace chargelab {
    class UploadInterface {
    public:
        enum class Result {
            kSucceeded,
            kFailed
        };

    public:
        virtual ~UploadInterface() = default;
        virtual Result upload(std::string const& uri, std::vector<uint8_t> const& content, bool append, std::function<void(std::size_t)> const& report_progress) = 0;
    };
}


#endif //CHARGELAB_OPEN_FIRMWARE_UPLOAD_INTERFACE_H
