#ifndef CHARGELAB_OPEN_FIRMWARE_JSON_H
#define CHARGELAB_OPEN_FIRMWARE_JSON_H

#include "openocpp/common/stream.h"
#include "openocpp/common/macro.h"
#include "openocpp/common/logging.h"

#include <optional>
#include <type_traits>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <variant>
#include <limits>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <unordered_set>

namespace chargelab {
    namespace json {
        struct NullType {};
        struct BoolType {bool value;};
        struct IntType {int value;};
        struct UintType {unsigned value;};
        struct Int64Type {std::int64_t value;};
        struct Uint64Type {std::uint64_t value;};
        struct DoubleType {double value;};
        struct RawNumberType {char const* str; std::size_t length; bool copy;};
        struct StringType {char const* str; std::size_t length; bool copy;};
        struct StartObjectType {};
        struct KeyType {char const* str; std::size_t length; bool copy;};
        struct EndObjectType {std::size_t memberCount;};
        struct StartArrayType {};
        struct EndArrayType {std::size_t elementCount;};
        using TokenType = std::variant <
                NullType,
                BoolType,
                IntType,
                UintType,
                Int64Type,
                Uint64Type,
                DoubleType,
                RawNumberType,
                StringType,
                StartObjectType,
                KeyType,
                EndObjectType,
                StartArrayType,
                EndArrayType
        >;

        std::string token_to_string(TokenType const& token) {
            if (std::holds_alternative<NullType>(token)) return "NullType";
            if (std::holds_alternative<BoolType>(token)) return std::string("BoolType{") + (std::get<BoolType>(token).value ? "true" : "false") + "}";
            if (std::holds_alternative<IntType>(token)) return "IntType{" + std::to_string(std::get<IntType>(token).value) + "}";
            if (std::holds_alternative<UintType>(token)) return "UintType{" + std::to_string(std::get<UintType>(token).value) + "}";
            if (std::holds_alternative<Int64Type>(token)) return "Int64Type{" + std::to_string(std::get<Int64Type>(token).value) + "}";
            if (std::holds_alternative<Uint64Type>(token)) return "Uint64Type{" + std::to_string(std::get<Uint64Type>(token).value) + "}";
            if (std::holds_alternative<DoubleType>(token)) return "DoubleType{" + std::to_string(std::get<DoubleType>(token).value) + "}";
            if (std::holds_alternative<RawNumberType>(token)) return "RawNumberType{" + std::string(std::get<RawNumberType>(token).str) + "}";
            if (std::holds_alternative<StringType>(token)) return "StringType{" + std::string(std::get<StringType>(token).str) + "}";
            if (std::holds_alternative<StartObjectType>(token)) return "StartObjectType";
            if (std::holds_alternative<KeyType>(token)) return "KeyType{" + std::string(std::get<KeyType>(token).str) + "}";
            if (std::holds_alternative<EndObjectType>(token)) return "EndObjectType";
            if (std::holds_alternative<StartArrayType>(token)) return "StartArrayType";
            if (std::holds_alternative<EndArrayType>(token)) return "EndArrayType";
            return "Unknown";
        }

        std::string token_to_string(std::optional<TokenType> const& token) {
            if (!token.has_value())
                return "nullopt";

            return token_to_string(token.value());
        }

        class SaxToTokenHandler {
        public:
            using char_type = char;
            using size_type = std::size_t;

        public:
            bool Null() {current_ = NullType{}; return true;}
            bool Bool(bool value) {current_ = BoolType{value}; return true;}
            bool Int(int value) {current_ = IntType{value}; return true;}
            bool Uint(unsigned value) {current_ = UintType{value}; return true;}
            bool Int64(int64_t value) {current_ = Int64Type{value}; return true;}
            bool Uint64(uint64_t value) {current_ = Uint64Type{value}; return true;}
            bool Double(double value) {current_ = DoubleType{value}; return true;}
            bool RawNumber(const char_type* str, size_type length, bool copy) { current_ = RawNumberType{str, length, copy}; return true;}
            bool String(const char_type* str, size_type length, bool copy) { current_ = StringType{str, length, copy}; return true;}
            bool StartObject() {current_ = StartObjectType{}; return true;}
            bool Key(const char_type* str, size_type length, bool copy) { current_ = KeyType{str, length, copy}; return true;}
            bool EndObject(size_type memberCount) { current_ = EndObjectType{memberCount}; return true;}
            bool StartArray() {current_ = StartArrayType{}; return true;}
            bool EndArray(size_type elementCount) { current_ = EndArrayType{elementCount}; return true;}

            [[nodiscard]] TokenType const& getToken() const {
                return current_;
            }

        private:
            TokenType current_ = NullType{};
        };

        class ByteWriterAdapter {
        public:
            typedef char Ch;

        public:
            ByteWriterAdapter(ByteWriterInterface& writer)
                : writer_(writer)
            {
            }

            void Put(Ch c) {
                writer_.put(c);
            }

            void Flush() {
                writer_.flush();
            }

            // Not implemented
            char Peek() const { assert(false); return 0; }
            char Take() { assert(false); return 0; }
            size_t Tell() const { assert(false); return 0; }
            char* PutBegin() { assert(false); return 0; }
            size_t PutEnd(char*) { assert(false); return 0; }

        private:
            stream::BufferedByteWriter writer_;
        };

        class ByteReaderAdapter {
        public:
            typedef char Ch;

        public:
            explicit ByteReaderAdapter(ByteReaderInterface& reader)
                : reader_(reader)
            {
            }

            Ch Peek() const {
                auto const result = reader_.peek();
                if (result == EOF)
                    return '\0';

                return (Ch)result;
            }

            Ch Take() {
                auto const result = reader_.take();
                if (result == EOF)
                    return '\0';

                return (Ch)result;
            }

            std::size_t Tell() const {
                return reader_.tellg();
            }

            // Not implemented
            void Put(Ch) { assert(false); }
            void Flush() { assert(false); }
            Ch* PutBegin() { assert(false); return 0; }
            size_t PutEnd(Ch*) { assert(false); return 0; }

        private:
            stream::BufferedByteReader reader_;
        };

        class JsonReader {
        public:
            JsonReader(ByteReaderInterface& input)
                : input_(input)
            {
                reader_.IterativeParseInit();
            }

            std::optional<TokenType> nextToken() {
                if (next_.has_value()) {
                    std::optional<TokenType> result = std::nullopt;
                    std::swap(result, next_);
                    return result;
                }

                if (reader_.HasParseError())
                    return std::nullopt;
                if (reader_.IterativeParseComplete())
                    return std::nullopt;
                if (!reader_.IterativeParseNext<rapidjson::kParseDefaultFlags>(input_, handler_))
                    return std::nullopt;

                return handler_.getToken();
            }

            std::optional<TokenType> peekToken() {
                if (next_.has_value())
                    return next_;

                if (reader_.HasParseError())
                    return std::nullopt;
                if (reader_.IterativeParseComplete())
                    return std::nullopt;
                if (!reader_.IterativeParseNext<rapidjson::kParseDefaultFlags>(input_, handler_))
                    return std::nullopt;

                return next_ = handler_.getToken();
            }

        private:
            ByteReaderAdapter input_;
            SaxToTokenHandler handler_;
            rapidjson::Reader  reader_;
            std::optional<TokenType> next_ = std::nullopt;
        };

        class JsonWriter {
        public:
            using char_type = char;
            using size_type = std::size_t;

        public:
            JsonWriter(ByteWriterInterface& output)
                : output_(output),
                // TODO: Is this safe?
                  writer_(output_)
            {
            }

            bool Null() {return writer_.Null();}
            bool Bool(bool value) {return writer_.Bool(value);}
            bool Int(int value) {return writer_.Int(value);}
            bool Uint(unsigned value) {return writer_.Uint(value);}
            bool Int64(int64_t value) {return writer_.Int64(value);}
            bool Uint64(uint64_t value) {return writer_.Uint64(value);}
            bool Double(double value) {return writer_.Double(value);}
            bool RawNumber(const char_type *str, size_type length) {return writer_.RawNumber(str, length);}
            bool String(const char_type *str, size_type length) {return writer_.String(str, length);}
            bool StartObject() {return writer_.StartObject();}
            bool Key(const char_type *str, size_type length) {return writer_.Key(str, length);}
            bool EndObject() {return writer_.EndObject();}
            bool StartArray() {return writer_.StartArray();}
            bool EndArray() {return writer_.EndArray();}

            bool RawNumber(const char_type* str) {return RawNumber(str, std::strlen(str));}
            bool RawNumber(std::string const& str) {return RawNumber(str.data(), str.size());}
            bool Key(const char_type *str) {return Key(str, std::strlen(str));}
            bool Key(std::string const& str) {return Key(str.data(), str.size());}
            bool String(const char_type* str) {return String(str, std::strlen(str));}
            bool String(std::string const& str) {return String(str.data(), str.size());}

        private:
            ByteWriterAdapter output_;
            rapidjson::Writer<ByteWriterAdapter> writer_;
        };

        template <typename T>
        bool expect_type(JsonReader& reader)
        {
            auto token = reader.nextToken();
            if (!token.has_value())
                return false;

            return std::holds_alternative<T>(token.value());
        }

        template <typename T, typename U=bool>
        struct Validate {
            static bool validate(T const&) {
                return true;
            }
        };

        template <typename T>
        struct Validate <std::optional<T>, bool> {
            static bool validate(std::optional<T> const& value) {
                if (!value.has_value())
                    return true;

                return Validate<T>::validate(value.value());
            }
        };

        template <typename T>
        struct Validate <std::vector<T>, bool> {
            static bool validate(std::vector<T> const& value) {
                for (auto const& x : value) {
                    if (!Validate<T>::validate(x))
                        return false;
                }

                return true;
            }
        };

        template <typename T>
        struct Validate <std::set<T>, bool> {
            static bool validate(std::set<T> const& value) {
                for (auto const& x : value) {
                    if (!Validate<T>::validate(x))
                        return false;
                }

                return true;
            }
        };

        template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
        struct Validate <std::unordered_set<Key, Hash, KeyEqual, Allocator>, bool> {
            static bool validate(std::unordered_set<Key, Hash, KeyEqual, Allocator> const& value) {
                for (auto const& x : value) {
                    if (!Validate<Key>::validate(x))
                        return false;
                }

                return true;
            }
        };

        template <typename Key, typename Value>
        struct Validate <std::map<Key, Value>, bool> {
            static bool validate(std::map<Key, Value> const& value) {
                for (auto const& x : value) {
                    if (!Validate<Key>::validate(x.first))
                        return false;
                    if (!Validate<Value>::validate(x.second))
                        return false;
                }

                return true;
            }
        };

        template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
        struct Validate <std::unordered_map<Key, T, Hash, KeyEqual, Allocator>, bool> {
            static bool validate(std::unordered_map<Key, T, Hash, KeyEqual, Allocator> const& value) {
                for (auto const& x : value) {
                    if (!Validate<Key>::validate(x.first))
                        return false;
                    if (!Validate<T>::validate(x.second))
                        return false;
                }

                return true;
            }
        };

        template <typename T, typename U=void>
        struct ReadPrimitive;

        template <>
        struct ReadPrimitive <bool, void> {
            static bool read_json(JsonReader& reader, bool& value) {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();
                if (!std::holds_alternative<BoolType>(token)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Unexpected type - expected bool: " << token_to_string(token);
                    return false;
                }

                value = std::get<BoolType>(token).value;
                return true;
            }
        };

        template <typename T>
        struct ReadPrimitive <T, typename std::enable_if<std::is_integral<T>::value>::type> {
            template <typename TT, typename UU = void>
            struct traits;

            template <typename TT>
            struct traits <TT, typename std::enable_if<std::is_integral<TT>::value>::type> {
                static constexpr bool is_signed = std::is_signed<TT>::value;
                static constexpr auto make_unsigned(TT const& value) {
                    return std::make_unsigned_t<TT>(value);
                }
            };

            template <typename TT>
            struct traits <TT, typename std::enable_if<std::is_floating_point<TT>::value>::type> {
                static constexpr bool is_signed = true;
                static constexpr TT make_unsigned(TT const& value) {
                    return std::abs(value);
                }
            };

            template<class Lhs, class Rhs>
            static constexpr  bool cmp_less(Lhs lhs, Rhs rhs) noexcept
            {
                if constexpr (traits<Lhs>::is_signed == traits<Rhs>::is_signed)
                    return lhs < rhs;
                else if constexpr (traits<Lhs>::is_signed)
                    return lhs < 0 || traits<Lhs>::make_unsigned(lhs) < rhs;
                else
                    return rhs >= 0 && lhs < traits<Rhs>::make_unsigned(rhs);
            }

            template <typename Container>
            static bool read_integer(TokenType const& token, T& value) {
                if (!std::holds_alternative<Container>(token))
                    return false;

                auto const& container = std::get<Container>(token);
                if (cmp_less(container.value, std::numeric_limits<T>::min())) {
                    value = std::numeric_limits<T>::min();
                } else if (cmp_less(std::numeric_limits<T>::max(), container.value)) {
                    value = std::numeric_limits<T>::max();
                } else {
                    value = static_cast<T>(container.value);
                }

                return true;
            }

            template <typename Container>
            static bool read_floating_point(TokenType const& token, T& value) {
                if (!std::holds_alternative<Container>(token))
                    return false;

                auto const& container = std::get<Container>(token);
                if (std::isnan(container.value)) {
                    value = 0;
                } else if (cmp_less(container.value, std::numeric_limits<T>::min())) {
                    value = std::numeric_limits<T>::min();
                } else if (cmp_less(std::numeric_limits<T>::max(), container.value)) {
                    value = std::numeric_limits<T>::max();
                } else {
                    value = static_cast<T>(container.value);
                }

                return true;
            }

            static bool read_json(JsonReader& reader, T& value) {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();

                // First try integer types
                if (read_integer<IntType>(token, value)) return true;
                if (read_integer<UintType>(token, value)) return true;
                if (read_integer<Int64Type>(token, value)) return true;
                if (read_integer<Uint64Type>(token, value)) return true;

                if (read_floating_point<DoubleType>(token, value)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Unexpected type - converting double to integer type: " << token_to_string(token);
                    return true;
                }

                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected type - expecting numeric type for field: " << token_to_string(token);
                return false;
            }
        };

        template <typename T>
        struct ReadPrimitive <T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
            template <typename Container>
            static bool read_token(TokenType const& token, T& value) {
                if (!std::holds_alternative<Container>(token))
                    return false;

                value = static_cast<T>(std::get<Container>(token).value);
                return true;
            }

            static bool read_json(JsonReader& reader, T& value) {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();

                // Note: integer or floating point types are valid here
                if (read_token<IntType>(token, value)) return true;
                if (read_token<UintType>(token, value)) return true;
                if (read_token<Int64Type>(token, value)) return true;
                if (read_token<Uint64Type>(token, value)) return true;
                if (read_token<DoubleType>(token, value)) return true;

                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected type - expecting numeric type for field: " << token_to_string(token);
                return false;
            }
        };

        template <>
        struct ReadPrimitive<std::string, void> {
            static bool read_json(JsonReader& reader, std::string& value) {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();
                if (!std::holds_alternative<StringType>(token)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Unexpected type - expecting string type for field" << token_to_string(token);
                    return false;
                }

                auto const& container = std::get<StringType>(token);
                value = std::string {container.str, container.length};
                return true;
            };
        };

        template <typename T, typename U=void>
        struct WritePrimitive;

        template <>
        struct WritePrimitive <bool, void> {
            static void write_json(JsonWriter& writer, bool const& value) {
                writer.Bool(value);
            }
        };

        template <typename T>
        struct WritePrimitive <T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && sizeof(T) <= sizeof(unsigned)>::type> {
            static void write_json(JsonWriter& writer, T const& value) {
                writer.Uint(static_cast<unsigned>(value));
            }
        };

        template <typename T>
        struct WritePrimitive <T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && (sizeof(T) > sizeof(unsigned))>::type> {
            static void write_json(JsonWriter& writer, T const& value) {
                writer.Uint64(static_cast<uint64_t>(value));
            }
        };

        template <typename T>
        struct WritePrimitive <T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) <= sizeof(int)>::type> {
            static void write_json(JsonWriter& writer, T const& value) {
                writer.Int(static_cast<int>(value));
            }
        };

        template <typename T>
        struct WritePrimitive <T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value && (sizeof(T) > sizeof(int))>::type> {
            static void write_json(JsonWriter& writer, T const& value) {
                writer.Int64(static_cast<int64_t>(value));
            }
        };

        template <typename T>
        struct WritePrimitive <T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
            static void write_json(JsonWriter& writer, T const& value) {
                writer.Double(static_cast<double>(value));
            }
        };

        template <>
        struct WritePrimitive <std::string, void> {
            static void write_json(JsonWriter& writer, std::string const& value) {
                writer.String(value.data(), value.size());
            }
        };

        template <typename T, typename U=bool>
        struct ReadValue {
            static bool read_json(JsonReader& reader, T& value) {
                return ReadPrimitive<T>::read_json(reader, value);
            }
        };

        template <typename T>
        struct ReadValue <T, decltype(T::read_json(std::declval<JsonReader&>(), std::declval<T&>()))> {
            static bool read_json(JsonReader& reader, T& value) {
                if (!T::read_json(reader, value))
                    return false;

                if (!Validate<T>::validate(value)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed validating field while reading: " << value;
                    // TODO: Add a global to turn on/off enforcing validation and failing deserialization here
                }

                return true;
            }
        };

        template <typename T>
        struct ReadValue <std::optional<T>, bool> {
            static bool read_json(JsonReader& reader, std::optional<T>& value) {
                auto const next = reader.peekToken();
                if (!next.has_value())
                    return false;

                if (std::holds_alternative<NullType>(next.value())) {
                    reader.nextToken();
                    value = std::nullopt;
                    return true;
                } else {
                    value = T{};
                    return ReadValue<T>::read_json(reader, value.value());
                }
            }
        };

        template <typename T, typename Allocator>
        struct ReadValue <std::vector<T, Allocator>, bool> {
            static bool read_json(JsonReader& reader, std::vector<T, Allocator>& value) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                value.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndArrayType>(next.value()))
                        break;

                    T entry;
                    if (!ReadValue<T>::read_json(reader, entry))
                        return false;

                    value.push_back(std::move(entry));
                }

                if (!expect_type<EndArrayType>(reader))
                    return false;

                return true;
            }
        };

        template <typename T, typename Allocator>
        struct ReadValue <std::set<T, Allocator>, bool> {
            static bool read_json(JsonReader& reader, std::set<T, Allocator>& value) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                value.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndArrayType>(next.value()))
                        break;

                    T entry;
                    if (!ReadValue<T>::read_json(reader, entry))
                        return false;

                    value.insert(std::move(entry));
                }

                if (!expect_type<EndArrayType>(reader))
                    return false;

                return true;
            }
        };

        template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
        struct ReadValue <std::unordered_set<Key, Hash, KeyEqual, Allocator>, bool> {
            static bool read_json(JsonReader& reader, std::unordered_set<Key, Hash, KeyEqual, Allocator>& value) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                value.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndArrayType>(next.value()))
                        break;

                    Key entry;
                    if (!ReadValue<Key>::read_json(reader, entry))
                        return false;

                    value.insert(std::move(entry));
                }

                if (!expect_type<EndArrayType>(reader))
                    return false;

                return true;
            }
        };

        template <typename T, typename Compare, typename Allocator>
        struct ReadValue <std::map<std::string, T, Compare, Allocator>, bool> {
            static bool read_json(JsonReader& reader, std::map<std::string, T, Compare, Allocator>& value) {
                if (!expect_type<StartObjectType>(reader))
                    return false;

                value.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndObjectType>(next.value()))
                        break;

                    auto const key_container = reader.nextToken();
                    if (!key_container.has_value())
                        return false;
                    if (!std::holds_alternative<KeyType>(key_container.value()))
                        return false;

                    auto const& key_token = std::get<KeyType>(key_container.value());
                    std::string key {key_token.str, key_token.length};

                    T entry;
                    if (!ReadValue<T>::read_json(reader, entry))
                        return false;

                    value[key] = std::move(entry);
                }

                if (!expect_type<EndObjectType>(reader))
                    return false;

                return true;
            }
        };

        template <typename Key, typename T, typename Compare, typename Allocator>
        struct ReadValue <
                std::map<Key, T, Compare, Allocator>,
                typename std::enable_if<!std::is_same<Key, std::string>::value, bool>::type
        > {
            static bool read_json(JsonReader& reader, std::map<Key, T, Compare, Allocator>& container) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                container.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndArrayType>(next.value()))
                        break;

                    Key key;
                    T value;
                    if (!expect_type<StartArrayType>(reader))
                        return false;
                    if (!ReadValue<Key>::read_json(reader, key))
                        return false;
                    if (!ReadValue<T>::read_json(reader, value))
                        return false;
                    if (!expect_type<EndArrayType>(reader))
                        return false;

                    container[key] = std::move(value);
                }

                if (!expect_type<EndArrayType>(reader))
                    return false;

                return true;
            }
        };

        template <typename T, typename Hash, typename KeyEqual, typename Allocator>
        struct ReadValue <std::unordered_map<std::string, T, Hash, KeyEqual, Allocator>, bool> {
            static bool read_json(JsonReader& reader, std::unordered_map<std::string, T, Hash, KeyEqual, Allocator>& value) {
                if (!expect_type<StartObjectType>(reader))
                    return false;

                value.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndObjectType>(next.value()))
                        break;

                    auto const key_container = reader.nextToken();
                    if (!key_container.has_value())
                        return false;
                    if (!std::holds_alternative<KeyType>(key_container.value()))
                        return false;

                    auto const& key_token = std::get<KeyType>(key_container.value());
                    std::string key {key_token.str, key_token.length};

                    T entry;
                    if (!ReadValue<T>::read_json(reader, entry))
                        return false;

                    value[key] = std::move(entry);
                }

                if (!expect_type<EndObjectType>(reader))
                    return false;

                return true;
            }
        };

        template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
        struct ReadValue <
                std::unordered_map<Key, T, Hash, KeyEqual, Allocator>,
                typename std::enable_if<!std::is_same<Key, std::string>::value, bool>::type
        > {
            static bool read_json(JsonReader& reader, std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& container) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                container.clear();
                while (true) {
                    auto const next = reader.peekToken();
                    if (!next.has_value())
                        return false;
                    if (std::holds_alternative<EndArrayType>(next.value()))
                        break;

                    Key key;
                    T value;
                    if (!expect_type<StartArrayType>(reader))
                        return false;
                    if (!ReadValue<Key>::read_json(reader, key))
                        return false;
                    if (!ReadValue<T>::read_json(reader, value))
                        return false;
                    if (!expect_type<EndArrayType>(reader))
                        return false;

                    container[key] = std::move(value);
                }

                if (!expect_type<EndArrayType>(reader))
                    return false;

                return true;
            }
        };

        template <>
        struct ReadValue <SystemTimeMillis, bool> {
            static bool read_json(JsonReader& reader, SystemTimeMillis& value) {
                std::int64_t number {};
                if (!ReadPrimitive<std::int64_t>::read_json(reader, number))
                    return false;

                value = static_cast<SystemTimeMillis>(number);
                return true;
            }
        };

        template <>
        struct ReadValue <SteadyPointMillis, bool> {
            static bool read_json(JsonReader& reader, SteadyPointMillis& value) {
                std::int64_t number {};
                if (!ReadPrimitive<std::int64_t>::read_json(reader, number))
                    return false;

                value = static_cast<SteadyPointMillis>(number);
                return true;
            }
        };

        template <typename K, typename V>
        struct ReadValue<std::pair<K, V>, bool> {
            static bool read_json(JsonReader& reader, std::pair<K, V>& value) {
                if (!expect_type<StartArrayType>(reader))
                    return false;

                K first;
                V second;

                if (!ReadValue<K>::read_json(reader, first))
                    return false;
                if (!ReadValue<V>::read_json(reader, second))
                    return false;

                if (!expect_type<EndArrayType>(reader))
                    return false;

                value = std::make_pair(std::move(first), std::move(second));
                return true;
            }
        };

        template <typename T, typename U=void>
        struct WriteValue {
            static void write_json(JsonWriter& writer, T const& value) {
                WritePrimitive<T>::write_json(writer, value);
            }
        };

        template <typename T>
        struct WriteValue <T, std::void_t<decltype(T::write_json(std::declval<JsonWriter&>(), std::declval<T const&>()))>> {
            static void write_json(JsonWriter& writer, T const& value) {
                if (!Validate<T>::validate(value)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed validating field while writing: " << value;
                    // TODO: Add a global to turn on/off enforcing validation and failing deserialization here
                }

                T::write_json(writer, value);
            }
        };

        template <typename T>
        struct WriteValue <std::optional<T>, void> {
            static void write_json(JsonWriter& writer, std::optional<T> const& value) {
                if (value.has_value()) {
                    WriteValue<T>::write_json(writer, value.value());
                } else {
                    writer.Null();
                }
            }
        };

        template <typename T, typename Allocator>
        struct WriteValue <std::vector<T, Allocator>, void> {
            static void write_json(JsonWriter& writer, std::vector<T, Allocator> const& value) {
                writer.StartArray();
                for (auto const& x : value)
                    WriteValue<T>::write_json(writer, x);
                writer.EndArray();
            }
        };

        template <typename T, typename Allocator>
        struct WriteValue <std::set<T, Allocator>, void> {
            static void write_json(JsonWriter& writer, std::set<T, Allocator> const& value) {
                writer.StartArray();
                for (auto const& x : value)
                    WriteValue<T>::write_json(writer, x);
                writer.EndArray();
            }
        };

        template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
        struct WriteValue <std::unordered_set<Key, Hash, KeyEqual, Allocator>, void> {
            static void write_json(JsonWriter& writer, std::unordered_set<Key, Hash, KeyEqual, Allocator> const& value) {
                writer.StartArray();
                for (auto const& x : value)
                    WriteValue<Key>::write_json(writer, x);
                writer.EndArray();
            }
        };

        template <typename T, typename Compare, typename Allocator>
        struct WriteValue <std::map<std::string, T, Compare, Allocator>, void> {
            static void write_json(JsonWriter& writer, std::map<std::string, T, Compare, Allocator> const& value) {
                writer.StartObject();
                for (auto const& x : value) {
                    writer.Key(x.first.data(), x.first.size());
                    WriteValue<T>::write_json(writer, x.second);
                }
                writer.EndObject();
            }
        };

        template <typename Key, typename T, typename Compare, typename Allocator>
        struct WriteValue <
                std::map<Key, T, Compare, Allocator>,
                typename std::enable_if<!std::is_same<Key, std::string>::value>::type
        > {
            static void write_json(JsonWriter& writer, std::map<Key, T, Compare, Allocator> const& value) {
                writer.StartArray();
                for (auto const& x : value) {
                    writer.StartArray();
                    WriteValue<Key>::write_json(writer, x.first);
                    WriteValue<T>::write_json(writer, x.second);
                    writer.EndArray();
                }
                writer.EndArray();
            }
        };

        template <typename T, typename Hash, typename KeyEqual, typename Allocator>
        struct WriteValue <std::unordered_map<std::string, T, Hash, KeyEqual, Allocator>, void> {
            static void write_json(JsonWriter& writer, std::unordered_map<std::string, T, Hash, KeyEqual, Allocator> const& value) {
                writer.StartObject();
                for (auto const& x : value) {
                    writer.Key(x.first.data(), x.first.size());
                    WriteValue<T>::write_json(writer, x.second);
                }
                writer.EndObject();
            }
        };

        template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
        struct WriteValue <
                std::unordered_map<Key, T, Hash, KeyEqual, Allocator>,
                typename std::enable_if<!std::is_same<Key, std::string>::value>::type
        > {
            static void write_json(JsonWriter& writer, std::unordered_map<Key, T, Hash, KeyEqual, Allocator> const& value) {
                writer.StartArray();
                for (auto const& x : value) {
                    writer.StartArray();
                    WriteValue<Key>::write_json(writer, x.first);
                    WriteValue<T>::write_json(writer, x.second);
                    writer.EndArray();
                }
                writer.EndArray();
            }
        };

        template <>
        struct WriteValue <SystemTimeMillis, void> {
            static void write_json(JsonWriter& writer, SystemTimeMillis const& value) {
                writer.Int64(static_cast<std::int64_t>(value));
            }
        };

        template <>
        struct WriteValue <SteadyPointMillis, void> {
            static void write_json(JsonWriter& writer, SteadyPointMillis const& value) {
                writer.Int64(static_cast<std::int64_t>(value));
            }
        };
        
        template <>
        struct WriteValue <logging::LogLevel, void> {
            static void write_json(JsonWriter& writer, logging::LogLevel const& value) {
                switch (value) {
                    default: writer.String("bad-level"); break;
                    case logging::LogLevel::trace: writer.String("trace"); break;
                    case logging::LogLevel::debug: writer.String("debug"); break;
                    case logging::LogLevel::info: writer.String("info"); break;
                    case logging::LogLevel::warning: writer.String("warning"); break;
                    case logging::LogLevel::error: writer.String("error"); break;
                    case logging::LogLevel::fatal: writer.String("fatal"); break;
                }
            }
        };

        template <typename K, typename V>
        struct WriteValue <std::pair<K,V>, void> {
            static void write_json(JsonWriter& writer, std::pair<K, V> const& value) {
                writer.StartArray();
                WriteValue<K>::write_json(writer, value.first);
                WriteValue<V>::write_json(writer, value.second);
                writer.EndArray();
            }
        };

        template <typename T, typename U=bool>
        struct IncludeField {
            static bool include_field(T const&) {
                return true;
            }

            static bool is_required() {
                return true;
            }
        };

        template <typename T>
        struct IncludeField <T, decltype(T::include_field(std::declval<T const&>()))> {
            static bool include_field(T const& value) {
                return T::include_field(value);
            }

            static bool is_required() {
                return T::is_required();
            }
        };

        template <typename T>
        struct IncludeField <std::optional<T>, bool> {
            static bool include_field(std::optional<T> const& value) {
                return value.has_value();
            }

            static bool is_required() {
                return false;
            }
        };

        bool skip_field(JsonReader& reader) {
            int object_stack = 0;
            int array_stack = 0;
            do {
                auto const& next = reader.nextToken();
                if (!next.has_value())
                    return false;

                auto const& token = next.value();
                if (std::holds_alternative<StartObjectType>(token)) {
                    object_stack++;
                } else if (std::holds_alternative<EndObjectType>(token)) {
                    object_stack--;
                } else if (std::holds_alternative<StartArrayType>(token)) {
                    array_stack++;
                } else if (std::holds_alternative<EndArrayType>(token)) {
                    array_stack--;
                }
            } while (object_stack > 0 || array_stack > 0);

            return true;
        }

        bool names_match_ci_ignore_ws(char const* lhs, std::size_t lhs_len, char const* rhs, std::size_t rhs_len) {
            std::size_t index_lhs = 0;
            std::size_t index_rhs = 0;

            while (index_lhs < lhs_len) {
                auto const ch_lhs = lhs[index_lhs++];
                if (std::isspace(ch_lhs))
                    continue;

                while (true) {
                    if (index_rhs >= rhs_len)
                        return false;

                    auto const ch_rhs = rhs[index_rhs++];
                    if (std::isspace(ch_rhs))
                        continue;

                    if (std::tolower(ch_lhs) != std::tolower(ch_rhs))
                        return false;

                    break;
                }
            }

            while (index_rhs < rhs_len) {
                auto const ch_rhs = rhs[index_rhs++];
                if (!std::isspace(ch_rhs))
                    return false;
            }

            return true;
        }

        bool is_field(KeyType const& key, char const* name) {
            return names_match_ci_ignore_ws(key.str, key.length, name, std::strlen(name));
        }

        bool is_enum(char const* key, std::size_t key_len, char const* entry) {
            return names_match_ci_ignore_ws(key, key_len, entry, std::strlen(entry));
        }
    }

    template <typename T>
    bool read_json(ByteReaderInterface& input, T& value) {
        json::JsonReader reader(input);
        return json::ReadValue<T>::read_json(reader, value);
    }

    template <typename T>
    bool read_json(ByteReaderInterface&& input, T& value) {
        return read_json(input, value);
    }

    template <typename T>
    void write_json(ByteWriterInterface& output, T const& value) {
        json::JsonWriter writer(output);
        json::WriteValue<T>::write_json(writer, value);
    }

    template <typename T>
    void write_json(ByteWriterInterface&& output, T const& value) {
        write_json(output, value);
    }

    template <typename T>
    std::string write_json_to_string(T const& value) {
        stream::StringWriter output;
        write_json(output, value);
        return output.str();
    }

    template <typename T>
    std::optional<T> read_json_from_string(std::string_view const& text) {
        T result {};
        stream::StringReader reader {std::string{text}};
        if (!read_json(reader, result))
            return std::nullopt;

        return std::move(result);
    }

    template <typename T>
    std::size_t calculate_size(T const& value) {
        stream::SizeCalculator calculator;
        write_json(calculator, value);
        return calculator.getTotalBytes();
    }

    template <typename T>
    std::optional<T> read_field_from_object(std::string const& json, std::string const& key) {
        stream::StringReader input_stream {json};
        json::JsonReader reader {input_stream};

        int object_stack = 0;
        int array_stack = 0;
        do {
            auto const& next = reader.nextToken();
            if (!next.has_value())
                break;

            auto const& token = next.value();
            if (std::holds_alternative<json::StartObjectType>(token)) {
                object_stack++;
            } else if (std::holds_alternative<json::EndObjectType>(token)) {
                object_stack--;
            } else if (std::holds_alternative<json::StartArrayType>(token)) {
                array_stack++;
            } else if (std::holds_alternative<json::EndArrayType>(token)) {
                array_stack--;
            } else if (std::holds_alternative<json::KeyType>(token)) {
                auto const& x = std::get<json::KeyType>(token);
                if (object_stack == 1 && key == std::string{x.str, x.length}) {
                    T result {};
                    json::ReadValue<T>::read_json(reader, result);
                    return result;
                }
            }
        } while (object_stack > 0 || array_stack > 0);

        return std::nullopt;
    }

    template <typename T>
    std::string insert_into_object(std::string const& json, std::string const& key, T const& value) {
        stream::StringReader input_stream {json};
        json::JsonReader reader {input_stream};

        stream::StringWriter output_stream;
        json::JsonWriter writer {output_stream};

        int object_stack = 0;
        int array_stack = 0;
        bool replaced_field = false;
        do {
            auto const& next = reader.nextToken();
            if (!next.has_value())
                break;

            auto const& token = next.value();
            if (std::holds_alternative<json::StartObjectType>(token)) {
                writer.StartObject();
                object_stack++;
            } else if (std::holds_alternative<json::EndObjectType>(token)) {
                if (object_stack == 0 && !replaced_field) {
                    writer.Key(key.data(), key.size());
                    json::WriteValue<T>::write_json(writer, value);
                }

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
                if (object_stack == 1 && key == std::string{x.str, x.length}) {
                    json::skip_field(reader);
                    json::WriteValue<T>::write_json(writer, value);
                    replaced_field = true;
                }
            } else {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected JSON token";
            }
        } while (object_stack > 0 || array_stack > 0);

        return output_stream.str();
    }

    namespace logging {
        template <typename T>
        struct LogWriter <T,
                typename std::enable_if<
                    std::is_class<T>::value,
                    std::void_t<decltype(write_json_to_string(std::declval<T const&>()))>
                >::type
        > {
            static void write(std::string& accumulator, T const& value) {
                accumulator += write_json_to_string(value);
            }
        };
    };
}

#define CHARGELAB_SET_REQUIRED(FIELD)                                                           \
    {                                                                                           \
        using type = typename std::decay<decltype(value.FIELD)>::type;                          \
        missing[index] = ::chargelab::json::IncludeField<type>::is_required();                  \
        index++;                                                                                \
    }

#define CHARGELAB_READ_FIELD(FIELD)                                                             \
    if (::chargelab::json::is_field(key, #FIELD)) {                                             \
        using type = typename std::decay<decltype(value.FIELD)>::type;                          \
        if (!::chargelab::json::ReadValue<type>::read_json(reader, value.FIELD))                \
            return false;                                                                       \
        missing[index] = false;                                                                 \
        continue;                                                                               \
    }                                                                                           \
    index++;

#define CHARGELAB_WRITE_FIELD(FIELD)                                                            \
    {                                                                                           \
        using type = typename std::decay<decltype(value.FIELD)>::type;                          \
        if (::chargelab::json::IncludeField<type>::include_field(value.FIELD)) {                \
            writer.Key(#FIELD);                                                                 \
            ::chargelab::json::WriteValue<type>::write_json(writer, value.FIELD);               \
        }                                                                                       \
    }

#define CHARGELAB_OPERATOR_EQUAL(FIELD) if((FIELD) != rhs.FIELD) return false;
#define CHARGELAB_JSON_INTRUSIVE(TYPE, ...)                                                     \
    static bool read_json(::chargelab::json::JsonReader& reader, TYPE& value) {                 \
        if (!::chargelab::json::expect_type<::chargelab::json::StartObjectType>(reader))        \
            return false;                                                                       \
                                                                                                \
        std::array<bool,CHARGELAB_NUM_ARGS(__VA_ARGS__)> missing{};                             \
        int index = 0;                                                                          \
        CHARGELAB_PASTE(CHARGELAB_SET_REQUIRED, __VA_ARGS__)                                    \
                                                                                                \
        while (true) {                                                                          \
            auto const& next = reader.nextToken();                                              \
            if (!next.has_value())                                                              \
                return false;                                                                   \
                                                                                                \
            auto const& token = next.value();                                                   \
            if (std::holds_alternative<::chargelab::json::EndObjectType>(token))                \
                break;                                                                          \
            if (!std::holds_alternative<::chargelab::json::KeyType>(token))                     \
                return false;                                                                   \
                                                                                                \
            auto const& key = std::get<::chargelab::json::KeyType>(token);                      \
            index = 0;                                                                          \
            CHARGELAB_PASTE(CHARGELAB_READ_FIELD, __VA_ARGS__)                                  \
            if (!::chargelab::json::skip_field(reader))                                         \
                return false;                                                                   \
        }                                                                                       \
                                                                                                \
        if (!std::all_of(missing.begin(), missing.end(), [](auto const& x) {return !x;}))       \
            return false;                                                                       \
                                                                                                \
        return true;                                                                            \
    }                                                                                           \
    static void write_json(::chargelab::json::JsonWriter& writer, TYPE const& value) {          \
        writer.StartObject();                                                                   \
        CHARGELAB_PASTE(CHARGELAB_WRITE_FIELD, __VA_ARGS__)                                     \
        writer.EndObject();                                                                     \
    }                                                                                           \
    bool operator==(TYPE const& rhs) const {                                                    \
        CHARGELAB_PASTE(CHARGELAB_OPERATOR_EQUAL, __VA_ARGS__)                                  \
        return true;                                                                            \
    }                                                                                           \
    bool operator!=(TYPE const& rhs) const {                                                    \
        return !operator==(rhs);                                                                \
    }

#define CHARGELAB_JSON_INTRUSIVE_EMPTY(TYPE)                                                    \
    static bool read_json(::chargelab::json::JsonReader& reader, TYPE& value) {                 \
        if (!::chargelab::json::expect_type<::chargelab::json::StartObjectType>(reader))        \
            return false;                                                                       \
                                                                                                \
        while (true) {                                                                          \
            auto const& next = reader.nextToken();                                              \
            if (!next.has_value())                                                              \
                return false;                                                                   \
                                                                                                \
            auto const& token = next.value();                                                   \
            if (std::holds_alternative<::chargelab::json::EndObjectType>(token))                \
                break;                                                                          \
            if (!std::holds_alternative<::chargelab::json::KeyType>(token))                     \
                return false;                                                                   \
            if (!::chargelab::json::skip_field(reader))                                         \
                return false;                                                                   \
        }                                                                                       \
                                                                                                \
        return true;                                                                            \
    }                                                                                           \
    static void write_json(::chargelab::json::JsonWriter& writer, TYPE const& value) {          \
        writer.StartObject();                                                                   \
        writer.EndObject();                                                                     \
    }                                                                                           \
    bool operator==(TYPE const& rhs) const {                                                    \
        return true;                                                                            \
    }                                                                                           \
    bool operator!=(TYPE const& rhs) const {                                                    \
        return !operator==(rhs);                                                                \
    }

#define CHARGELAB_ENUM_DEFINE(KEY) ,k ## KEY
#define CHARGELAB_ENUM_TO_STRING(KEY) case k ## KEY: return #KEY;
#define CHARGELAB_ENUM_FROM_STRING(KEY) if (::chargelab::json::is_enum(str, len, #KEY)) return k ## KEY;
#define CHARGELAB_JSON_ENUM(TYPE, ...)                                                          \
    class TYPE {                                                                                \
    public:                                                                                     \
        enum Value {kValueNotFoundInEnum CHARGELAB_PASTE(CHARGELAB_ENUM_DEFINE, __VA_ARGS__)};  \
                                                                                                \
        TYPE() = default;                                                                       \
        constexpr TYPE(Value value) : value_(value) {}                                          \
        constexpr operator Value() const {return value_;}                                       \
        explicit operator bool() const = delete;                                                \
                                                                                                \
        static bool read_json(::chargelab::json::JsonReader& reader, TYPE& value) {             \
            auto const& next = reader.nextToken();                                              \
            if (!next.has_value())                                                              \
                return false;                                                                   \
                                                                                                \
            auto const& token = next.value();                                                   \
            if (!std::holds_alternative<::chargelab::json::StringType>(token))                  \
                return false;                                                                   \
                                                                                                \
            auto const& key = std::get<::chargelab::json::StringType>(token);                   \
            auto const entry = from_string(key.str, key.length);                                \
            if (entry.value_ == kValueNotFoundInEnum)                                           \
                return false;                                                                   \
                                                                                                \
            value = entry;                                                                      \
            return true;                                                                        \
        }                                                                                       \
        static void write_json(::chargelab::json::JsonWriter& writer, TYPE const& value) {      \
            writer.String(value.to_string());                                                   \
        }                                                                                       \
                                                                                                \
        std::string to_string() const {                                                         \
            switch (value_) {                                                                   \
                CHARGELAB_PASTE(CHARGELAB_ENUM_TO_STRING, __VA_ARGS__)                          \
                                                                                                \
                default:                                                                        \
                case kValueNotFoundInEnum: return "ValueNotFoundInEnum";                        \
            }                                                                                   \
        }                                                                                       \
        static TYPE from_string(char const* str, std::size_t len) {                             \
            CHARGELAB_PASTE(CHARGELAB_ENUM_FROM_STRING, __VA_ARGS__)                            \
            return kValueNotFoundInEnum;                                                        \
        }                                                                                       \
        static TYPE from_string(std::string_view const& str) {                                  \
            return from_string(str.data(), str.size());                                         \
        }                                                                                       \
                                                                                                \
    private:                                                                                    \
        Value value_ {};                                                                        \
    };

#define CHARGELAB_JSON_ENUM_CUSTOM(TYPE, ...)                                                   \
    public:                                                                                     \
        TYPE() = default;                                                                       \
        constexpr TYPE(Value value) : value_(value) {}                                          \
        constexpr operator Value() const {return value_;}                                       \
        explicit operator bool() const = delete;                                                \
                                                                                                \
        static bool read_json(::chargelab::json::JsonReader& reader, TYPE& value) {             \
            auto const& next = reader.nextToken();                                              \
            if (!next.has_value())                                                              \
                return false;                                                                   \
                                                                                                \
            auto const& token = next.value();                                                   \
            if (!std::holds_alternative<::chargelab::json::StringType>(token))                  \
                return false;                                                                   \
                                                                                                \
            auto const& key = std::get<::chargelab::json::StringType>(token);                   \
            value = from_string(key.str, key.length);                                           \
            return true;                                                                        \
        }                                                                                       \
        static void write_json(::chargelab::json::JsonWriter& writer, TYPE const& value) {      \
            writer.String(value.to_string());                                                   \
        }                                                                                       \
                                                                                                \
        std::string to_string() const {                                                         \
            const std::pair<Value, char const*> m[] = __VA_ARGS__;                              \
            auto const value = value_;                                                          \
            auto it = std::find_if(                                                             \
                std::begin(m),                                                                  \
                std::end(m),                                                                    \
                [value](auto const& x) -> bool                                                  \
            {                                                                                   \
                return x.first == value;                                                        \
            });                                                                                 \
                                                                                                \
            if (it != std::end(m)) {                                                            \
                return it->second;                                                              \
            } else {                                                                            \
                return m[0].second;                                                             \
            }                                                                                   \
        }                                                                                       \
        static TYPE from_string(char const* str, std::size_t len) {                             \
            const std::pair<Value, char const*> m[] = __VA_ARGS__;                              \
            auto it = std::find_if(                                                             \
                std::begin(m),                                                                  \
                std::end(m),                                                                    \
                [str, len](auto const& x) -> bool                                               \
            {                                                                                   \
                return ::chargelab::json::is_enum(str, len, x.second);                          \
            });                                                                                 \
                                                                                                \
            if (it != std::end(m)) {                                                            \
                return it->first;                                                               \
            } else {                                                                            \
                return m[0].first;                                                              \
            }                                                                                   \
        }                                                                                       \
        static TYPE from_string(std::string_view const& str) {                                  \
            return from_string(str.data(), str.size());                                         \
        }                                                                                       \
                                                                                                \
    private:                                                                                    \
        Value value_ {};

#define CHARGELAB_JSON_INTRUSIVE_CALL(TYPE, ACTION_ID, ...)                                     \
    static constexpr ActionId kActionId = ActionId::ACTION_ID;                                  \
    CHARGELAB_JSON_INTRUSIVE(TYPE, __VA_ARGS__)
#define CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(TYPE, ACTION_ID)                                    \
    static constexpr ActionId kActionId = ActionId::ACTION_ID;                                  \
    CHARGELAB_JSON_INTRUSIVE_EMPTY(TYPE)

#define CHARGELAB_JSON_AS_PRIMITIVE(TYPE, FIELD)                                                \
    static bool read_json(::chargelab::json::JsonReader& reader, TYPE& value) {                 \
        using type = typename std::decay<decltype(value.FIELD)>::type;                          \
        return ::chargelab::json::ReadPrimitive<type>::read_json(reader, value.FIELD);          \
    }                                                                                           \
    static void write_json(::chargelab::json::JsonWriter& writer, TYPE const& value) {          \
        using type = typename std::decay<decltype(value.FIELD)>::type;                          \
        ::chargelab::json::WritePrimitive<type>::write_json(writer, value.FIELD);               \
    }                                                                                           \
    bool operator==(TYPE const& rhs) const {                                                    \
        return true;                                                                            \
    }                                                                                           \
    bool operator!=(TYPE const& rhs) const {                                                    \
        return !operator==(rhs);                                                                \
    }

#endif //CHARGELAB_OPEN_FIRMWARE_JSON_H
