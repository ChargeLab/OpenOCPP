#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_VECTOR_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_VECTOR_H

#include <variant>

#include "openocpp/helpers/json.h"

namespace chargelab {
    template <typename T, bool Optional = false, int MinElements = 0, int MaxElements = -1>
    struct Vector {
    public:
        using this_type = Vector<T, Optional, MinElements, MaxElements>;
        using data_type = std::vector<T>;
        using visitor_type = std::function<void(std::function<void(T const&)>)>;

    public:
        Vector() {}
        Vector(data_type data) : supplier_(std::move(data)) {}
        Vector(visitor_type visitor) : supplier_(std::move(visitor)) {}
        Vector(const this_type& other) = default;
        this_type& operator=(const this_type& other) = default;

        data_type value() const {
            if (std::holds_alternative<data_type>(supplier_)) {
                return std::get<data_type>(supplier_);
            } else {
                std::vector<T> data;
                std::get<visitor_type>(supplier_)([&](auto const& x) {
                    data.push_back(x);
                });

                return data;
            }
        }

        template <typename Visitor>
        void visit(Visitor&& visitor) const {
            if (std::holds_alternative<data_type>(supplier_)) {
                auto const& data = std::get<data_type>(supplier_);
                for (auto const& x : data)
                    visitor(x);
            } else {
                std::get<visitor_type>(supplier_)(std::forward<Visitor>(visitor));
            }
        }

        std::size_t size() const {
            std::size_t result = 0;
            visit([&](auto const&) {
                result++;
            });

            return result;
        }

        static bool read_json(json::JsonReader& reader, this_type& value) {
            auto next = reader.peekToken();
            if (!next.has_value())
                return false;

            if (std::holds_alternative<json::NullType>(next.value())) {
                value.supplier_ = data_type {};
            } else {
                data_type data;
                json::ReadValue<data_type>::read_json(reader, data);
                value.supplier_ = std::move(data);
            }

            return true;
        }

        static void write_json(json::JsonWriter& writer, this_type const& value) {
            if (std::holds_alternative<data_type>(value.supplier_)) {
                json::WriteValue<data_type>::write_json(writer, std::get<data_type>(value.supplier_));
            } else {
                writer.StartArray();
                std::get<visitor_type>(value.supplier_)([&](auto const& element) {
                    json::WriteValue<T>::write_json(writer, element);
                });
                writer.EndArray();
            }
        }

        static bool include_field(this_type const& value) {
            return !Optional || value.size() > 0;
        }

        static bool is_required() {
            return !Optional;
        }

        static bool validate(this_type const& value) {
            auto const size = value.size();
            if (MinElements >= 0) {
                if (size < MinElements)
                    return false;
            }
            if (MaxElements >= 0) {
                if (size > MaxElements)
                    return false;
            }

            bool valid = true;
            value.visit([&](auto const& element) {
                if (!valid)
                    return;

                valid = json::Validate<T>::validate(element);
            });

            return valid;
        }

        friend bool operator==(this_type const& lhs, this_type const& rhs) {
            auto const& lhs_data = lhs.value();
            auto const& rhs_data = rhs.value();
            if (lhs_data.size() != rhs_data.size())
                return false;

            for (auto it_lhs=lhs_data.begin(), it_rhs=rhs_data.begin(); it_lhs != lhs_data.end(); ++it_lhs, ++it_rhs) {
                if (*it_lhs != *it_rhs)
                    return false;
            }

            return true;
        }

        friend bool operator!=(this_type const& lhs, this_type const& rhs) {
            return !(lhs == rhs);
        }

    private:
        std::variant<data_type, visitor_type> supplier_ = data_type {};
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_VECTOR_H
