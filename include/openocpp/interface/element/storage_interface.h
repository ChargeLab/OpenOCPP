#ifndef CHARGELAB_OPEN_FIRMWARE_STORAGE_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_STORAGE_INTERFACE_H

#include <cstdio>
#include <functional>

namespace chargelab {
    class StorageInterface {
    public:
        virtual ~StorageInterface() = default;

        virtual bool read(std::function<bool(FILE*)> const& function) = 0;
        virtual bool write(std::function<bool(FILE*)> const& function) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STORAGE_INTERFACE_H
