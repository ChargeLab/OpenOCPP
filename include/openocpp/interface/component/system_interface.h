#ifndef CHARGELAB_OPEN_FIRMWARE_SYSTEM_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_SYSTEM_INTERFACE_H

#include "openocpp/model/system_types.h"
#include "openocpp/interface/element/storage_interface.h"
#include "openocpp/interface/element/flash_block_interface.h"

#include <cstdint>
#include <memory>

namespace chargelab {
    class SystemInterface {
    public:
        virtual ~SystemInterface() = default;

        virtual SystemTimeMillis systemClockNow() = 0;
        virtual SteadyPointMillis steadyClockNow() = 0;

        virtual void setSystemClock(SystemTimeMillis now) = 0;
        virtual void resetSoft() = 0;
        virtual void resetHard() = 0;

        virtual bool isClockOutOfSync() = 0;

        virtual std::unique_ptr<chargelab::StorageInterface> getStorage(std::string const& file_name) = 0;
        virtual std::unique_ptr<chargelab::FlashBlockInterface> getPartition(std::string const& label) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_SYSTEM_INTERFACE_H
