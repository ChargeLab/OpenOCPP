#ifndef CHARGELAB_OPEN_FIRMWARE_RAW_JSON_H
#define CHARGELAB_OPEN_FIRMWARE_RAW_JSON_H

#include "openocpp/helpers/json.h"

#include <string>
#include <memory>

namespace chargelab::common {
    namespace detail {
        class RawJsonInterface {
        public:
            virtual ~RawJsonInterface() = default;
            virtual std::string data() const = 0;
            virtual void write_json(json::JsonWriter& writer) const = 0;
        };

        template <typename T>
        class RawJsonLazy : public RawJsonInterface {
        public:
            RawJsonLazy(T value) : value_(std::move(value)) {}

            std::string data() const override {
                return write_json_to_string(value_);
            }

            void write_json(json::JsonWriter& writer) const override {
                json::WriteValue<T>::write_json(writer, value_);
            }

        private:
            T value_;
        };
    }

    class RawJson {
    private:
        using text_type = std::string;
        using lazy_type = std::shared_ptr<detail::RawJsonInterface>;
        using data_type = std::variant<text_type, lazy_type>;

    public:
        RawJson() = default;
        RawJson(std::string data) : data_(std::move(data)) {}
        RawJson(std::shared_ptr<detail::RawJsonInterface> data) : data_(std::move(data)) {}

        bool operator==(RawJson const& rhs) const {
            return data() == rhs.data();
        }

        bool operator!=(RawJson const& rhs) const {
            return !operator==(rhs);
        }

        std::string data() const {
            if (std::holds_alternative<text_type>(data_)) {
                return std::get<text_type>(data_);
            } else {
                return std::get<lazy_type>(data_)->data();
            }
        }

        static bool read_json(json::JsonReader& reader, RawJson& value) {
            stream::StringWriter stream;
            json::JsonWriter writer {stream};

            int object_stack = 0;
            int array_stack = 0;
            do {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();
                if (std::holds_alternative<json::StartObjectType>(token)) {
                    writer.StartObject();
                    object_stack++;
                } else if (std::holds_alternative<json::EndObjectType>(token)) {
                    writer.EndObject();
                    object_stack--;
                } else if (std::holds_alternative<json::StartArrayType>(token)) {
                    writer.StartArray();
                    array_stack++;
                } else if (std::holds_alternative<json::EndArrayType>(token)) {
                    writer.EndArray();
                    array_stack--;
                } else if (std::holds_alternative<json::NullType>(token)) {
                    writer.Null();
                } else if (std::holds_alternative<json::BoolType>(token)) {
                    auto const& x = std::get<json::BoolType>(token);
                    writer.Bool(x.value);
                } else if (std::holds_alternative<json::IntType>(token)) {
                    auto const& x = std::get<json::IntType>(token);
                    writer.Int(x.value);
                } else if (std::holds_alternative<json::UintType>(token)) {
                    auto const& x = std::get<json::UintType>(token);
                    writer.Uint(x.value);
                } else if (std::holds_alternative<json::Int64Type>(token)) {
                    auto const& x = std::get<json::Int64Type>(token);
                    writer.Int64(x.value);
                } else if (std::holds_alternative<json::Uint64Type>(token)) {
                    auto const& x = std::get<json::Uint64Type>(token);
                    writer.Uint64(x.value);
                } else if (std::holds_alternative<json::DoubleType>(token)) {
                    auto const& x = std::get<json::DoubleType>(token);
                    writer.Double(x.value);
                } else if (std::holds_alternative<json::RawNumberType>(token)) {
                    auto const& x = std::get<json::RawNumberType>(token);
                    writer.RawNumber(x.str, x.length);
                } else if (std::holds_alternative<json::StringType>(token)) {
                    auto const& x = std::get<json::StringType>(token);
                    writer.String(x.str, x.length);
                } else if (std::holds_alternative<json::KeyType>(token)) {
                    auto const& x = std::get<json::KeyType>(token);
                    writer.Key(x.str, x.length);
                } else {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected JSON token";
                }
            } while (object_stack > 0 || array_stack > 0);

            value.data_ = stream.str();
            return true;
        }

        static void write_json(json::JsonWriter& writer, RawJson const& value) {
            if (std::holds_alternative<lazy_type>(value.data_)) {
                std::get<lazy_type>(value.data_)->write_json(writer);
                return;
            }

            stream::StringReader stream {std::get<text_type>(value.data_)};
            json::JsonReader reader {stream};
            while (true) {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    break;

                auto const& token = next.value();
                if (std::holds_alternative<json::StartObjectType>(token)) {
                    writer.StartObject();
                } else if (std::holds_alternative<json::EndObjectType>(token)) {
                    writer.EndObject();
                } else if (std::holds_alternative<json::StartArrayType>(token)) {
                    writer.StartArray();
                } else if (std::holds_alternative<json::EndArrayType>(token)) {
                    writer.EndArray();
                } else if (std::holds_alternative<json::NullType>(token)) {
                    writer.Null();
                } else if (std::holds_alternative<json::BoolType>(token)) {
                    auto const& x = std::get<json::BoolType>(token);
                    writer.Bool(x.value);
                } else if (std::holds_alternative<json::IntType>(token)) {
                    auto const& x = std::get<json::IntType>(token);
                    writer.Int(x.value);
                } else if (std::holds_alternative<json::UintType>(token)) {
                    auto const& x = std::get<json::UintType>(token);
                    writer.Uint(x.value);
                } else if (std::holds_alternative<json::Int64Type>(token)) {
                    auto const& x = std::get<json::Int64Type>(token);
                    writer.Int64(x.value);
                } else if (std::holds_alternative<json::Uint64Type>(token)) {
                    auto const& x = std::get<json::Uint64Type>(token);
                    writer.Uint64(x.value);
                } else if (std::holds_alternative<json::DoubleType>(token)) {
                    auto const& x = std::get<json::DoubleType>(token);
                    writer.Double(x.value);
                } else if (std::holds_alternative<json::RawNumberType>(token)) {
                    auto const& x = std::get<json::RawNumberType>(token);
                    writer.RawNumber(x.str, x.length);
                } else if (std::holds_alternative<json::StringType>(token)) {
                    auto const& x = std::get<json::StringType>(token);
                    writer.String(x.str, x.length);
                } else if (std::holds_alternative<json::KeyType>(token)) {
                    auto const& x = std::get<json::KeyType>(token);
                    writer.Key(x.str, x.length);
                } else {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected JSON token";
                }
            }
        }

        template <typename T>
        static RawJson from_value(T const& value) {
            RawJson result {};

            stream::StringReader stream {write_json_to_string(value)};
            json::JsonReader reader {stream};

            if (!read_json(reader, result)) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed creating RawJson from record";
                return RawJson {};
            }

            return result;
        }

        template <typename T>
        static RawJson from_value_lazy(T const& value) {
            return RawJson{std::shared_ptr<detail::RawJsonInterface> {new detail::RawJsonLazy<T> {value}}};
        }

        static RawJson empty_object() {
            return RawJson{};
        }

    private:
        data_type data_ = "{}";
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_RAW_JSON_H
