#ifndef ESP32_FIRMWARE_TEST_FLASH_BLOCK_INTERFACE_H
#define ESP32_FIRMWARE_TEST_FLASH_BLOCK_INTERFACE_H

#include <functional>

namespace chargelab {
    class FlashBlockInterface {
    public:
        virtual ~FlashBlockInterface() = default;

        virtual bool read(std::size_t src_offset, void *dst, std::size_t size) const = 0;
        virtual bool write(std::size_t dst_offset, void *src, std::size_t size) = 0;
        virtual bool erase() = 0;
        [[nodiscard]] virtual std::size_t size() const = 0;
    };
}

#endif //ESP32_FIRMWARE_TEST_FLASH_BLOCK_INTERFACE_H
