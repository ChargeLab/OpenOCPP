#ifndef CHARGELAB_OPEN_FIRMWARE_BYTE_WRITER_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_BYTE_WRITER_INTERFACE_H

#include <string>
#include <cstring>

namespace chargelab {
    class ByteWriterInterface {
    public:
        virtual void write(char const *s, std::size_t count) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_BYTE_WRITER_INTERFACE_H
