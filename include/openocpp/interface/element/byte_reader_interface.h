#ifndef CHARGELAB_OPEN_FIRMWARE_BYTE_READER_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_BYTE_READER_INTERFACE_H

#include <string>
#include <cstring>

namespace chargelab {
    class ByteReaderInterface {
    public:
        virtual std::size_t read(char* s, std::size_t n) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_BYTE_READER_INTERFACE_H
