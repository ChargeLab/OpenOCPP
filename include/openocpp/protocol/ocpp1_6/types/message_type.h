#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    class MessageType {
    public:
        enum Value {
            kValueNotFoundInEnum = -1,
            kCall = 2,
            kCallResult = 3,
            kCallError = 4
        };

    public:
        MessageType() = default;
        constexpr MessageType(Value value) : value_(value) {}
        constexpr operator Value() const { return value_; }
        explicit operator bool() const = delete;

        static bool read_json(::chargelab::json::JsonReader &reader, MessageType &value) {
            int num;
            if (!json::ReadPrimitive<int>::read_json(reader, num))
                return false;

            switch (num) {
                case kCall: value = kCall; break;
                case kCallResult: value = kCallResult; break;
                case kCallError: value = kCallError; break;
                default: value = kValueNotFoundInEnum; break;
            }

            return true;
        }
        static void write_json(::chargelab::json::JsonWriter &writer, MessageType const &value) {
            writer.Int((int)value.value_);
        }
        bool operator==(MessageType const &rhs) const { return value_ == rhs.value_; }
        bool operator!=(MessageType const &rhs) const { return !operator==(rhs); }

    private:
        Value value_{};
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TYPE_H
