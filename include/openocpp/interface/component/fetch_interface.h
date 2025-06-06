#ifndef CHARGELAB_OPEN_FIRMWARE_FETCH_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_FETCH_INTERFACE_H

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <functional>

namespace chargelab {
    class FetchInterface {
    public:
        enum class Result {
            kSucceeded,
            kFailed
        };

    public:
        virtual ~FetchInterface() = default;
        virtual Result fetch(std::string const& uri, std::function<Result(std::uint8_t const*, std::size_t)> const& process_chunk) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_FETCH_INTERFACE_H
