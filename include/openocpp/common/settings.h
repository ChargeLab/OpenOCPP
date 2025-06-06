#ifndef CHARGELAB_OPEN_FIRMWARE_SETTINGS_H
#define CHARGELAB_OPEN_FIRMWARE_SETTINGS_H

#include "openocpp/helpers/optional.h"
#include "openocpp/helpers/string.h"
#include "openocpp/helpers/json.h"
#include "openocpp/helpers/file.h"
#include "openocpp/helpers/uri.h"
#include "openocpp/common/logging.h"
#include "openocpp/interface/element/storage_interface.h"

#include <memory>
#include <optional>
#include <utility>
#include <mutex>

#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_attribute_type.h"
#include "openocpp/protocol/ocpp2_0/types/measurand_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/tx_start_point_values.h"
#include "openocpp/protocol/ocpp2_0/types/tx_stop_point_values.h"
#include "openocpp/protocol/ocpp2_0/types/network_connection_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_characteristics_type.h"
#include "openocpp/protocol/common/small_string.h"

namespace chargelab {
    class SettingString;
    class SettingTransitionString;
    class SettingBool;
    class SettingInt;
    class SettingDouble;
    class Settings;

    template <typename ValueType>
    class SettingTypedDelimitedList;

    template <typename ValueType>
    class SettingTransitionTypedDelimitedList;

    namespace detail {
        class SettingsConstants {
        public:
            static constexpr int kMaxProfileSlots = 5;
        };

        class SharedSettingLock {
            friend class ::chargelab::SettingString;
            friend class ::chargelab::SettingTransitionString;
            friend class ::chargelab::SettingBool;
            friend class ::chargelab::SettingInt;
            friend class ::chargelab::SettingDouble;

            template <typename ValueType>
            friend class ::chargelab::SettingTypedDelimitedList;

            template <typename ValueType>
            friend class ::chargelab::SettingTransitionTypedDelimitedList;

        private:
            static inline std::recursive_mutex mutex_ {};
        };

        inline bool isValidMeasurandList(std::string const& text) {
            bool result = true;
            string::SplitVisitor(text, ",", [&](std::string const& value) {
                if (ocpp2_0::MeasurandEnumType::from_string(value) == ocpp2_0::MeasurandEnumType::kValueNotFoundInEnum)
                    result = false;
            });

            return result;
        }

        inline std::string rangeToSequenceList(int begin, int end, int increment=1) {
            std::string result;
            char const* ifs = "";
            for (int i=begin; i < end; i += increment) {
                result += ifs + std::to_string(i);
                ifs = ",";
            }
            return result;
        }
    };
    
    CHARGELAB_JSON_ENUM(SettingTransitionType,
        Connection
    )

    CHARGELAB_JSON_ENUM(FeatureProfileType, 
        Core,
        FirmwareManagement,
        LocalAuthListManagement,
        Reservation,
        SmartCharging,
        RemoteTrigger,
        Custom,
        CoreOptional
    )

    struct DeviceModel1_6 {
        std::string name;
    };

    struct DeviceModel2_0 {
        ocpp2_0::ComponentType component_type;
        ocpp2_0::VariableType variable_type;
        ocpp2_0::VariableCharacteristicsType variable_characteristics;
    };

    class SettingConfig {
    public:
        SettingConfig(
                bool allow_ocpp_read,
                bool allow_ocpp_write,
                bool include_in_save,
                bool reboot_required
        ) : allow_ocpp_read_(allow_ocpp_read),
            allow_ocpp_write_(allow_ocpp_write),
            reboot_required_(reboot_required),
            include_in_save_(include_in_save)
        {}

        [[nodiscard]] bool isAllowOcppRead() const {
            return allow_ocpp_read_;
        }

        [[nodiscard]] bool isAllowOcppWrite() const {
            return allow_ocpp_write_;
        }

        [[nodiscard]] bool isRebootRequired() const {
            return reboot_required_;
        }

        [[nodiscard]] bool isIncludeInSave() const {
            return include_in_save_;
        }

    public:
        static SettingConfig rwPolicy(bool reboot_required=false) {
            return {true, true, true, reboot_required};
        }

        static SettingConfig roPolicy(bool reboot_required=false) {
            return {true, false, true, reboot_required};
        }

        static SettingConfig woPolicy(bool reboot_required=false) {
            return {false, true, true, reboot_required};
        }

        static SettingConfig rwNotSavedPolicy(bool reboot_required=false) {
            return {true, true, false, reboot_required};
        }

        static SettingConfig roNotSavedPolicy(bool reboot_required=false) {
            return {true, false, false, reboot_required};
        }

        static SettingConfig woNotSavedPolicy(bool reboot_required=false) {
            return {false, true, false, reboot_required};
        }

    private:
        bool allow_ocpp_read_;
        bool allow_ocpp_write_;
        bool reboot_required_;
        bool include_in_save_;
    };

    struct SettingMetadata {
        std::string id;
        SettingConfig config;
        std::optional<DeviceModel1_6> model1_6;
        std::optional<DeviceModel2_0> model2_0;
        std::string default_value;
    };

    class SettingBase {
    public:
        using metadata_generator_type = SettingMetadata(*)();
        using metadata_pointer_type = std::unique_ptr<SettingMetadata>;
        using metadata_container_type = std::variant<metadata_generator_type, metadata_pointer_type>;

    public:
        virtual ~SettingBase() = default;
        explicit SettingBase(metadata_container_type metadata)
                : metadata_(std::move(metadata))
        {
            if (std::holds_alternative<metadata_pointer_type>(metadata_)) {
                assert(std::get<metadata_pointer_type>(metadata_) != nullptr);
            }
        }

        [[nodiscard]] SettingMetadata getMetadata() const {
            if (std::holds_alternative<metadata_generator_type>(metadata_)) {
                return std::get<metadata_generator_type>(metadata_)();
            } else {
                return *std::get<metadata_pointer_type>(metadata_);
            }
        }

        [[nodiscard]] std::string getId() const {
            return getMetadata().id;
        }

        [[nodiscard]] SettingConfig getConfig() const {
            return getMetadata().config;
        }

        [[nodiscard]] std::optional<DeviceModel1_6> getModel16() const {
            return getMetadata().model1_6;
        }

        [[nodiscard]] std::optional<DeviceModel2_0> getModel20() const {
            return getMetadata().model2_0;
        }

        [[nodiscard]] bool getModified() const {
            return modified_;
        }

        virtual bool setValueFromString(const std::string &value) = 0;
        virtual std::string getValueAsString() const = 0;
        virtual bool load(std::string const& value) = 0;
        virtual std::string save() = 0;

        virtual bool hasPendingTransition(SettingTransitionType const&) {
            return false;
        }

        virtual bool startTransition(SettingTransitionType const&) {
            return true;
        }

        virtual void commit(SettingTransitionType const&) {
        }

        virtual void rollback(SettingTransitionType const&) {
        }

        virtual std::vector<ocpp2_0::VariableAttributeType> getAttributes2_0() const {
            auto const config = getConfig();

            ocpp2_0::MutabilityEnumType mutability;
            std::optional<ocpp2_0::StringPrimitive<2500>> value;
            if (config.isAllowOcppRead() && config.isAllowOcppWrite()) {
                mutability = ocpp2_0::MutabilityEnumType::kReadWrite;
                value = getValueAsString();
            } else if (config.isAllowOcppRead()) {
                mutability = ocpp2_0::MutabilityEnumType::kReadOnly;
                value = getValueAsString();
            } else if (config.isAllowOcppWrite()) {
                mutability = ocpp2_0::MutabilityEnumType::kWriteOnly;
            } else {
                return {};
            }

            return {
                    ocpp2_0::VariableAttributeType{
                            ocpp2_0::AttributeEnumType::kActual,
                            value,
                            mutability,
                            config.isIncludeInSave()
                    }
            };
        }

        virtual bool setAttribute2_0(ocpp2_0::AttributeEnumType const& attribute_type, std::string const& value) {
            if (attribute_type != ocpp2_0::AttributeEnumType::kActual)
                return false;

            return setValueFromString(value);
        }

        void setModified(bool modified) {
            modified_ = modified;
        }

    private:
        metadata_container_type metadata_;

        // Note: default modified state is true. If this setting is read from a file or written to a file the flag is
        // cleared.
        std::atomic<bool> modified_ = true;
    };

    class SettingString : public SettingBase {
    public:
        using value_type = std::string;
        using validator_type = bool(*)(value_type const&);

    public:
        virtual ~SettingString() = default;
        explicit SettingString(metadata_container_type metadata, validator_type validator)
                : SettingBase(std::move(metadata)),
                  validator_(validator)
        {
            current_value_ = getMetadata().default_value;
        }

        template<typename T>
        SettingString(SettingConfig, T&&) = delete;
        SettingString(SettingString const&) = delete;
        SettingString& operator=(SettingString const&) = delete;

        [[nodiscard]] value_type getValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            return current_value_.value();
        }

        bool setValue(value_type const& value) {
            if (!validator_(value))
                return false;

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (current_value_ != value) {
                current_value_ = value;
                setModified(true);
            }
            return true;
        }

        bool setValueFromString(const std::string &value) override {
            return setValue(value);
        }

        [[nodiscard]] std::string getValueAsString() const override {
            return getValue();
        }

        bool load(const std::string &value) override {
            if (!validator_(value))
                return false;

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            current_value_ = value;
            setModified(false);
            return true;
        }

        std::string save() override {
            setModified(false);
            return getValue();
        }

    private:
        validator_type validator_;

        SmallString current_value_;
    };

    // Transition container to allow the station to move safely from one collection of critical settings to another. For
    // example, when moving over to another network I might need to update the following parameters:
    // - CentralSystemUrl
    // - ChargePointId
    // - ~BasicAuthPassword~
    // -- Note: this type of transaction doesn't work well with OCPP 2.0.1 requirements to reconnect immediately
    // I would ideally want to set those parameters and restart the charger to apply them, with the charger rolling back
    // to all previous defaults if it wasn't able to connect with the new configuration.
    //
    // The model used here is:
    // - A setting can be updated freely while a "transition" is not taking place, however this only ever updates the
    //   "pending" value, never the current value.
    // - At some point the station will start a transition (for example: immediately after restarting)
    // - While a transition is ongoing writes to the transitioning variables will be prevented
    // - At some subsequent point the transition is determined to either succeed or fail, at which point all the setting
    //   transitions are committed (the pending value becomes the current value) or rolled back (in which case all
    //   pending values are discarded).
    class SettingTransitionString : public SettingBase {
    public:
        using value_type = std::string;
        using validator_type = bool(*)(value_type const&);

    public:
        virtual ~SettingTransitionString() = default;
        SettingTransitionString(
                metadata_container_type metadata,
                SettingTransitionType transition_type,
                validator_type validator
        )
                : SettingBase(std::move(metadata)),
                  transition_type_(transition_type),
                  validator_(validator)
        {
            current_value_ = getMetadata().default_value;
        }

        template<typename T>
        SettingTransitionString(SettingTransitionString, T&&) = delete;
        SettingTransitionString(SettingTransitionString const&) = delete;
        SettingTransitionString& operator=(SettingTransitionString const&) = delete;

        bool load(const std::string &value) override {
            auto const parsed = read_json_from_string<std::vector<std::string>>(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed loading " << getId() << " - data was not array: " << value;
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed loading " << getId() << " - transition in progress";
                return false;
            }

            auto const& array = parsed.value();
            if (!array.empty())
                current_value_ = array[0];
            if (array.size() >= 2)
                pending_value_ = array[1];

            return true;
        }

        std::string save() override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};

            std::vector<std::string> array {current_value_.value()};
            if (pending_value_.has_value())
                array.push_back(pending_value_->value());

            setModified(false);
            return write_json_to_string(array);
        }

        [[nodiscard]] value_type getValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value())
                return pending_value_->value();

            return current_value_.value();
        }

        [[nodiscard]] value_type transitionCurrentValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_in_progress_ && pending_value_.has_value())
                return pending_value_->value();

            return current_value_.value();
        }

        bool setValue(value_type const& value) {
            if (!validator_(value)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed setting " << getId() << " to '" << value << "' - validation failed";
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed setting " << getId() << " to '" << value << "' - a pending change was already in progress";
                return false;
            }

            if (value == current_value_.value()) {
                // Note: not setting the modified flag if nothing has changed
                if (!pending_value_.has_value())
                    return true;

                CHARGELAB_LOG_MESSAGE(info) << "Clearing pending value for setting/transition " << getId() << "/" << transition_type_;
                pending_value_ = std::nullopt;
                setModified(true);
                return true;
            } else {
                // Note: not setting the modified flag if nothing has changed
                if (pending_value_.has_value() && pending_value_.value() == value)
                    return true;

                CHARGELAB_LOG_MESSAGE(info) << "Setting pending value for setting/transition " << getId() << "/" << transition_type_ << " to: " << value;
                pending_value_ = value;
                setModified(true);
                return true;
            }
        }

        bool setValueFromString(const std::string &value) override {
            return setValue(value);
        }

        [[nodiscard]] std::string getValueAsString() const override {
            return getValue();
        }

        bool hasPendingTransition(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return false;
            if (!pending_value_.has_value())
                return false;

            CHARGELAB_LOG_MESSAGE(info) << "Transition was pending for " << getId() << ": " << current_value_.value() << " to " << pending_value_->value();
            return true;
        }

        // Returns true if the transition was started successfully, otherwise false (generally because another
        // transition was already in progress).
        bool startTransition(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return true;
            if (transition_in_progress_)
                return false;

            CHARGELAB_LOG_MESSAGE(warning) << "Starting transition for: " << getId();
            transition_in_progress_ = true;
            return true;
        }

        void commit(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return;
            if (!transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Commit called without a transition in progress";
                return;
            }

            transition_in_progress_ = false;
            if (pending_value_.has_value()) {
                current_value_ = pending_value_.value();
                pending_value_ = std::nullopt;
                setModified(true);
                CHARGELAB_LOG_MESSAGE(info) << "Committing value for setting/transition " << getId() << "/" << transition_type_ << ": " << current_value_.value();
            }
        }

        void rollback(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return;
            if (!transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Rollback called without a transition in progress";
                return;
            }

            transition_in_progress_ = false;
            pending_value_ = std::nullopt;
            setModified(true);
            CHARGELAB_LOG_MESSAGE(info) << "Rolling back value for setting/transition " << getId() << "/" << transition_type_ << " to: " << current_value_.value();
        }

        bool forceValue(std::string const& value) {
            if (!validator_(value)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed setting " << getId() << " to '" << value << "' - validation failed";
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            CHARGELAB_LOG_MESSAGE(info) << "Forcing value for setting " << getId() << " to: " << value;
            current_value_ = value;
            pending_value_ = std::nullopt;
            transition_in_progress_ = false;
            return true;
        }

    private:
        SettingTransitionType transition_type_;
        validator_type validator_;

        SmallString current_value_;
        std::optional<SmallString> pending_value_ {};
        bool transition_in_progress_ = false;
    };

    class SettingBool : public SettingBase {
    public:
        static constexpr char const* kTextTrue = "true";
        static constexpr char const* kTextFalse = "false";

    public:
        using value_type = bool;
        using validator_type = bool(*)(value_type const&);

    public:
        virtual ~SettingBool() = default;
        explicit SettingBool(metadata_container_type metadata, validator_type validator)
                : SettingBase(std::move(metadata)),
                  validator_(std::move(validator))
        {
            if (!setValueFromString(getMetadata().default_value)) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed setting default value for: " << getId();
            }
        }

        template<typename T>
        SettingBool(SettingConfig, T&&) = delete;
        SettingBool(SettingBool const&) = delete;
        SettingBool& operator=(SettingBool const&) = delete;

        [[nodiscard]] value_type getValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            return current_value_;
        }

        bool setValue(value_type value) {
            if (!validator_(value))
                return false;

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (current_value_ != value) {
                current_value_ = value;
                setModified(true);
            }
            return true;
        }

        bool setValueFromString(const std::string &value) override {
            if (string::EqualsIgnoreCaseAscii(value, kTextTrue)) {
                return setValue(true);
            } else if (string::EqualsIgnoreCaseAscii(value, kTextFalse)) {
                return setValue(false);
            } else {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad boolean text value: " << value;
                return false;
            }
        }

        [[nodiscard]] std::string getValueAsString() const override {
            if (getValue()) {
                return kTextTrue;
            } else {
                return kTextFalse;
            }
        }

        bool load(const std::string &value) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (string::EqualsIgnoreCaseAscii(value, kTextTrue)) {
                current_value_ = true;
                return true;
            } else if (string::EqualsIgnoreCaseAscii(value, kTextFalse)) {
                current_value_ = false;
                return true;
            } else {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad boolean text value: " << value;
                return false;
            }
        }

        std::string save() override {
            setModified(false);
            return getValueAsString();
        }

    private:
        validator_type validator_;

        value_type current_value_;
    };

    class SettingInt : public SettingBase {
    public:
        using value_type = int;
        using validator_type = bool(*)(value_type const&);

    public:
        virtual ~SettingInt() = default;
        explicit SettingInt(metadata_container_type metadata, validator_type validator)
                : SettingBase(std::move(metadata)),
                  validator_(validator)
        {
            if (!setValueFromString(getMetadata().default_value)) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed setting default value for: " << getId();
            }
        }

        template<typename T>
        SettingInt(SettingConfig, T&&) = delete;
        SettingInt(SettingInt const&) = delete;
        SettingInt& operator=(SettingInt const&) = delete;

        [[nodiscard]] value_type getValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            return current_value_;
        }

        bool setValue(value_type value) {
            if (!validator_(value))
                return false;

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (current_value_ != value) {
                current_value_ = value;
                setModified(true);
            }
            return true;
        }

        bool setValueFromString(const std::string &value) override {
            auto const parsed = string::ToInteger(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad integer text value: " << value;
                return false;
            }

            return setValue(parsed.value());
        }

        [[nodiscard]] std::string getValueAsString() const override {
            return std::to_string(getValue());
        }

        bool load(const std::string &value) override {
            auto const parsed = string::ToInteger(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad integer text value: " << value;
                return false;
            }
            if (!validator_(parsed.value())) {
                CHARGELAB_LOG_MESSAGE(warning) << "Validation failed for value: " << parsed.value();
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            current_value_ = parsed.value();
            return true;
        }

        std::string save() override {
            setModified(false);
            return getValueAsString();
        }

    private:
        validator_type validator_;

        std::atomic<value_type> current_value_;
    };

    class SettingDouble : public SettingBase {
    public:
        using value_type = double;
        using validator_type = bool(*)(value_type const&);

    public:
        virtual ~SettingDouble() = default;
        explicit SettingDouble(metadata_container_type metadata, validator_type validator)
                : SettingBase(std::move(metadata)),
                  validator_(validator)
        {
            if (!setValueFromString(getMetadata().default_value)) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed setting default value for: " << getId();
            }
        }

        template<typename T>
        SettingDouble(SettingConfig, T&&) = delete;
        SettingDouble(SettingDouble const&) = delete;
        SettingDouble& operator=(SettingDouble const&) = delete;

        [[nodiscard]] value_type getValue() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            return current_value_;
        }

        bool setValue(value_type value) {
            if (!validator_(value))
                return false;

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (current_value_ != value) {
                current_value_ = value;
                setModified(true);
            }
            return true;
        }

        bool setValueFromString(const std::string &value) override {
            auto const parsed = string::ToDouble(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad double text value: " << value;
                return false;
            }

            return setValue(parsed.value());
        }

        [[nodiscard]] std::string getValueAsString() const override {
            return std::to_string(getValue());
        }

        bool load(const std::string &value) override {
            auto const parsed = string::ToDouble(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad double text value: " << value;
                return false;
            }
            if (!validator_(parsed.value())) {
                CHARGELAB_LOG_MESSAGE(warning) << "Validation failed for value: " << parsed.value();
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            current_value_ = parsed.value();
            return true;
        }

        [[nodiscard]] std::string save() override {
            setModified(false);
            return getValueAsString();
        }

    private:
        validator_type validator_;

        value_type current_value_;
    };

    template <typename ValueType>
    class SettingTransitionTypedDelimitedList : public SettingBase {
    public:
        using value_type = ValueType;
        using container_type = std::vector<std::optional<value_type>>;
        using validator_type = bool(*)(container_type const&);

    public:
        virtual ~SettingTransitionTypedDelimitedList() = default;
        explicit SettingTransitionTypedDelimitedList(
                metadata_container_type metadata,
                SettingTransitionType transition_type,
                char const* field_separator,
                std::size_t max_size,
                validator_type validator
        )
                : SettingBase(std::move(metadata)),
                  transition_type_(transition_type),
                  field_separator_(field_separator),
                  max_size_(max_size),
                  validator_(std::move(validator))
        {
            current_value_ = parseFromValue(getMetadata().default_value);
        }

        template<typename T>
        SettingTransitionTypedDelimitedList(SettingConfig, T&&) = delete;
        SettingTransitionTypedDelimitedList(SettingTransitionTypedDelimitedList<ValueType> const&) = delete;
        SettingTransitionTypedDelimitedList& operator=(SettingTransitionTypedDelimitedList<ValueType> const&) = delete;

        [[nodiscard]] std::size_t size() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value()) {
                return pending_value_->size();
            } else {
                return current_value_.size();
            }
        }

        [[nodiscard]] std::optional<value_type> getValue(std::size_t index) const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value()) {
                if (pending_value_->size() <= index)
                    return std::nullopt;

                return pending_value_.value()[index];
            } else {
                if (current_value_.size() <= index)
                    return std::nullopt;

                return current_value_[index];
            }
        }

        bool setValue(std::size_t index, std::optional<value_type> const& value) {
            if (index >= max_size_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Index exceeded max size: " << index << " >= " << max_size_;
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value()) {
                auto update = pending_value_.value();
                if (index >= update.size())
                    update.resize(index+1);

                update[index] = value;
                if (!validator_(update))
                    return false;
                if (applyChange(pending_value_.value(), update))
                    setModified(true);
            } else {
                auto update = current_value_;
                if (index >= update.size())
                    update.resize(index+1);

                update[index] = value;
                if (!validator_(update))
                    return false;

                auto next = current_value_;
                if (applyChange(next, update)) {
                    pending_value_ = next;
                    setModified(true);
                }
            }

            return true;
        }

        /**
         * Commits any pending values then updates the current value directly.
         *
         * @param index
         * @param value
         * @return true on success, false on failure
         */
        bool forceValue(std::size_t const index, std::optional<value_type> const& value) {
            if (index >= max_size_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Index exceeded max size: " << index << " >= " << max_size_;
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value()) {
                current_value_ = pending_value_.value();
                setModified(true);
            }

            transition_in_progress_ = false;
            return forceCurrentValue(index, value);
        }

        /**
         * Updates the current value directly even if a pending transition was present. The pending transition is not
         * altered.
         *
         * @param index
         * @param value
         * @return true on success, false on failure
         */
        bool forceCurrentValue(std::size_t const index, std::optional<value_type> const& value) {
            if (index >= max_size_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Index exceeded max size: " << index << " >= " << max_size_;
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            auto update = current_value_;
            if (index >= update.size())
                update.resize(index+1);

            update[index] = value;
            if (!validator_(update))
                return false;

            if (applyChange(current_value_, update))
                setModified(true);

            return true;
        }

        bool setValueFromString(const std::string &container_value) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            auto const update = parseFromValue(container_value);
            if (!validator_(update))
                return false;
            if (!pending_value_.has_value()) {
                pending_value_ = container_type{};
                setModified(true);
            }
            if (applyChange(pending_value_.value(), update))
                setModified(true);

            return true;
        }

        [[nodiscard]] std::string getValueAsString() const override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (pending_value_.has_value()) {
                return containerToString(pending_value_.value());
            } else {
                return containerToString(current_value_);
            }
        }

        bool load(const std::string &value) override {
            auto const parsed = read_json_from_string<std::vector<std::string>>(value);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed loading " << getId() << " - data was not array: " << value;
                return false;
            }

            std::lock_guard lock {detail::SharedSettingLock::mutex_};
            if (transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed loading " << getId() << " - transition in progress";
                return false;
            }

            auto const array = parsed.value();
            if (!array.empty()) {
                auto const update = parseFromValue(array[0]);
                if (validator_(update))
                    applyChange(current_value_, update);
            }
            if (array.size() >= 2) {
                auto const update = parseFromValue(array[1]);
                if (validator_(update)) {
                    pending_value_ = container_type {};
                    applyChange(pending_value_.value(), update);
                }
            }

            setModified(false);
            return true;
        }

        std::string save() override {
            std::vector<std::string> array {containerToString(current_value_)};
            if (pending_value_.has_value())
                array.push_back(containerToString(pending_value_.value()));

            setModified(false);
            return write_json_to_string(array);
        }

        bool hasPendingTransition(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return false;
            if (!pending_value_.has_value())
                return false;

            CHARGELAB_LOG_MESSAGE(info) << "Transition was pending for " << getId() << ": " << current_value_ << " to " << pending_value_;
            return true;
        }

        // Returns true if the transition was started successfully, otherwise false (generally because another
        // transition was already in progress).
        bool startTransition(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return true;
            if (transition_in_progress_)
                return false;

            CHARGELAB_LOG_MESSAGE(warning) << "Starting transition for: " << getId();
            transition_in_progress_ = true;
            return true;
        }

        void commit(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return;
            if (!transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Commit called without a transition in progress";
                return;
            }

            transition_in_progress_ = false;
            if (pending_value_.has_value()) {
                current_value_ = pending_value_.value();
                pending_value_ = std::nullopt;
                setModified(true);
                CHARGELAB_LOG_MESSAGE(info) << "Committing value for setting/transition " << getId() << "/" << transition_type_ << ": " << current_value_;
            }
        }

        void rollback(SettingTransitionType const& transition_type) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_type != transition_type_)
                return;
            if (!transition_in_progress_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Rollback called without a transition in progress";
                return;
            }

            transition_in_progress_ = false;
            pending_value_ = std::nullopt;
            setModified(true);
            CHARGELAB_LOG_MESSAGE(info) << "Rolling back value for setting/transition " << getId() << "/" << transition_type_ << " to: " << current_value_;
        }

        [[nodiscard]] std::size_t transitionSize() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_in_progress_ && pending_value_.has_value()) {
                return pending_value_->size();
            } else {
                return current_value_.size();
            }
        }

        [[nodiscard]] std::optional<value_type> transitionCurrentValue(std::size_t index) const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (transition_in_progress_ && pending_value_.has_value()) {
                if (pending_value_->size() <= index)
                    return std::nullopt;

                return pending_value_.value()[index];
            } else {
                if (current_value_.size() <= index)
                    return std::nullopt;

                return current_value_[index];
            }
        }

    private:
        [[nodiscard]] std::string containerToString(container_type const& container) const {
            std::string result;

            char const* ifs = "";
            for (auto const& x : container) {
                result += ifs;
                ifs = field_separator_;

                if (x.has_value())
                    result += write_json_to_string(x.value());
            }

            return result;
        }

        container_type parseFromValue(std::string const& container_value) {
            container_type result;
            std::size_t index = 0;
            string::SplitVisitor(container_value, field_separator_, [&](std::string const& element_value) {
                auto const current_index = index++;
                if (element_value.empty())
                    return;

                auto const data = read_json_from_string<value_type>(element_value);
                if (!data.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing record in " << getId() << ": " << element_value;
                    return;
                }

                if (result.size() <= current_index)
                    result.resize(current_index+1);

                result[current_index] = data.value();
            });

            return result;
        }

        bool applyChange(container_type& current, container_type const& update) {
            bool changed = false;
            for (std::size_t i=0; i < update.size(); i++) {
                if (i >= current.size()) {
                    if (update[i].has_value()) {
                        changed = true;
                        break;
                    }
                } else if (current[i] != update[i]) {
                    changed = true;
                    break;
                }
            }

            if (!changed)
                return false;

            current.resize(update.size());
            for (std::size_t i=0; i < current.size(); i++)
                current[i] = update[i];

            return true;
        }

    private:
        SettingTransitionType transition_type_;
        char const* field_separator_;
        std::size_t max_size_;
        validator_type validator_;

        container_type current_value_;
        std::optional<container_type> pending_value_ {};
        bool transition_in_progress_ = false;
    };

    template <typename ValueType>
    class SettingTypedDelimitedList : public SettingBase {
    public:
        using value_type = ValueType;
        using container_type = std::vector<std::optional<value_type>>;
        using validator_type = bool(*)(container_type const&);

    public:
        virtual ~SettingTypedDelimitedList() = default;
        explicit SettingTypedDelimitedList(
                metadata_container_type metadata,
                char const* field_separator,
                std::size_t max_size,
                validator_type validator
        )
                : SettingBase(std::move(metadata)),
                  field_separator_(field_separator),
                  max_size_(max_size),
                  validator_(std::move(validator))
        {
            if (!setValueFromString(getMetadata().default_value)) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed setting default value for: " << getId();
            }
        }

        template<typename T>
        SettingTypedDelimitedList(SettingConfig, T&&) = delete;
        SettingTypedDelimitedList(SettingTypedDelimitedList<ValueType> const&) = delete;
        SettingTypedDelimitedList& operator=(SettingTypedDelimitedList<ValueType> const&) = delete;

        [[nodiscard]] std::size_t size() const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            return current_value_.size();
        }

        std::optional<value_type> getValue(std::size_t index) const {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            if (current_value_.size() <= index)
                return std::nullopt;

            return current_value_[index];
        }

        bool setValue(std::size_t index, std::optional<value_type> const& value) {
            if (index >= max_size_) {
                CHARGELAB_LOG_MESSAGE(warning) << "Index exceeded max size: " << index << " >= " << max_size_;
                return false;
            }

            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            auto update = current_value_;
            if (index >= update.size())
                update.resize(index+1);

            update[index] = value;
            if (!validator_(update))
                return false;
            if (applyChange(update))
                setModified(true);

            return true;
        }

        bool setValueFromString(const std::string &container_value) override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            auto const update = parseFromValue(container_value);
            if (!validator_(update))
                return false;
            if (applyChange(update))
                setModified(true);

            return true;
        }

        [[nodiscard]] std::string getValueAsString() const override {
            std::lock_guard lock{detail::SharedSettingLock::mutex_};
            std::string result;

            char const* ifs = "";
            for (auto const& x : current_value_) {
                result += ifs;
                ifs = field_separator_;

                if (x.has_value())
                    result += write_json_to_string(x.value());
            }

            return result;
        }

        bool load(const std::string &value) override {
            auto const update = parseFromValue(value);
            if (!validator_(update))
                return false;

            std::lock_guard lock {detail::SharedSettingLock::mutex_};
            applyChange(update);
            setModified(false);
            return true;
        }

        std::string save() override {
            setModified(false);
            return getValueAsString();
        }

    private:
        container_type parseFromValue(std::string const& container_value) {
            container_type result;
            std::size_t index = 0;
            string::SplitVisitor(container_value, field_separator_, [&](std::string const& element_value) {
                auto const current_index = index++;
                if (element_value.empty())
                    return;

                auto const data = read_json_from_string<value_type>(element_value);
                if (!data.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing record in " << getId() << ": " << element_value;
                    return;
                }

                if (result.size() <= current_index)
                    result.resize(current_index+1);

                result[current_index] = data.value();
            });

            return result;
        }

        bool applyChange(container_type const& update) {
            bool changed = false;
            for (std::size_t i=0; i < update.size(); i++) {
                if (i >= current_value_.size()) {
                    if (update[i].has_value()) {
                        changed = true;
                        break;
                    }
                } else if (current_value_[i] != update[i]) {
                    changed = true;
                    break;
                }
            }

            if (!changed)
                return false;

            current_value_.resize(update.size());
            for (std::size_t i=0; i < current_value_.size(); i++)
                current_value_[i] = update[i];

            return true;
        }

    private:
        char const* field_separator_;
        std::size_t max_size_;
        validator_type validator_;

        container_type current_value_;
    };

    class SettingVirtual : public SettingBase {
    public:
        using value_type = std::string;
        using get_value_type = std::string(*)(Settings*);
        using set_value_type = bool(*)(Settings*, std::string const&);

    public:
        virtual ~SettingVirtual() = default;
        explicit SettingVirtual(
                metadata_container_type metadata,
                Settings* this_ptr,
                get_value_type get_value,
                set_value_type set_value
        )
                : SettingBase(std::move(metadata)),
                  this_ptr_(this_ptr),
                  get_value_(get_value),
                  set_value_(set_value)
        {}

        template<typename T>
        SettingVirtual(SettingConfig, T&&) = delete;
        SettingVirtual(SettingVirtual const&) = delete;
        SettingVirtual& operator=(SettingVirtual const&) = delete;

        [[nodiscard]] std::string getValue() const {
            return get_value_(this_ptr_);
        }

        bool setValue(value_type const& value) {
            return set_value_(this_ptr_, value);
        }

        bool setValueFromString(const std::string &value) override {
            return setValue(value);
        }

        [[nodiscard]] std::string getValueAsString() const override {
            return getValue();
        }

        bool load(const std::string& value) override {
            // Note: this is used to load obsolete settings
            if (!value.empty())
                setValue(value);

            return true;
        }

        std::string save() override {
            CHARGELAB_LOG_MESSAGE(warning) << "Saving virtual setting: " << getId();
            return "";
        }

    private:
        Settings* this_ptr_;
        get_value_type get_value_;
        set_value_type set_value_;
    };

    struct SettingState {
        std::string id;
        SettingConfig config;
        std::optional<DeviceModel1_6> model1_6;
        std::optional<DeviceModel2_0> model2_0;
        std::string value;
    };

#ifdef CHARGELAB_DEVICE_MODEL
#error "Unexpected implementation name conflict"
#endif
#ifdef CHARGELAB_MERGE_NAMES
#error "Unexpected implementation name conflict"
#endif
#ifdef CHARGELAB_CREATE_CONFIG
#error "Unexpected implementation name conflict"
#endif
#ifdef CHARGELAB_VISIT_CONFIG
#error "Unexpected implementation name conflict"
#endif

#define CHARGELAB_DEVICE_MODEL(...) ::chargelab::DeviceModel{__VA_ARGS__}
#define CHARGELAB_MERGE_NAMES(x1, x2) x1 ## x2
#define CHARGELAB_CREATE_CONFIG(setting_type, name, setting, value, model) setting_type name{SettingConfig::setting, model, #name, value}
#define CHARGELAB_VISIT_CONFIG(name) visitor(name);

    namespace detail {
        struct SettingKeyValue {
            std::string key;
            std::string value;
            CHARGELAB_JSON_INTRUSIVE(SettingKeyValue, key, value)
        };
    };

    class Settings {
    public:
        // Standard OCPP 1.6 Settings
        SettingBool AllowOfflineTxForUnknownId {
                []() {
                    return SettingMetadata {
                            "AllowOfflineTxForUnknownId",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"AllowOfflineTxForUnknownId"},
                            DeviceModel2_0 {{"AuthCtrlr"}, {"OfflineTxForUnknownIdEnabled"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool AuthorizeRemoteTxRequests {
                []() {
                    return SettingMetadata {
                            "AuthorizeRemoteTxRequests",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"AuthorizeRemoteTxRequests"},
                            DeviceModel2_0 {{"AuthCtrlr"}, {"AuthorizeRemoteStart"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ClockAlignedDataInterval {
                []() {
                    return SettingMetadata {
                            "ClockAlignedDataInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"ClockAlignedDataInterval"},
                            DeviceModel2_0 {{"AlignedDataCtrlr"}, {"Interval"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt ConnectionTimeOut {
                []() {
                    return SettingMetadata {
                            "ConnectionTimeOut",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"ConnectionTimeOut"},
                            DeviceModel2_0 {{"TxCtrlr"}, {"EVConnectionTimeOut"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(90)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingString ConnectorPhaseRotation {
            []() {
                return SettingMetadata {
                        "ConnectorPhaseRotation",
                        SettingConfig::rwPolicy(false),
                        DeviceModel1_6 {"ConnectorPhaseRotation"},
                        std::nullopt,
                        "Unknown"
                };
            },
            [](auto const& value) {return value == "Unknown";}
        };

        SettingInt GetConfigurationMaxKeys {
                []() {
                    return SettingMetadata {
                            "GetConfigurationMaxKeys",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"GetConfigurationMaxKeys"},
                            std::nullopt,
                            std::to_string(100)
                    };
                },
                [](auto const& value) {return value > 0;}
        };

        SettingInt HeartbeatInterval {
                []() {
                    return SettingMetadata {
                            "HeartbeatInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"HeartbeatInterval"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"HeartbeatInterval"}, {"s", ocpp2_0::DataEnumType::kinteger, 1, 86400}},
                            std::to_string(30)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingBool LocalAuthorizeOffline {
                []() {
                    return SettingMetadata {
                            "LocalAuthorizeOffline",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"LocalAuthorizeOffline"},
                            DeviceModel2_0 {{"AuthCtrlr"}, {"LocalAuthorizeOffline"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool LocalPreAuthorize {
                []() {
                    return SettingMetadata {
                            "LocalPreAuthorize",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"LocalPreAuthorize"},
                            DeviceModel2_0 {{"AuthCtrlr"}, {"LocalPreAuthorize"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString MeterValuesAlignedData {
                []() {
                    return SettingMetadata {
                            "MeterValuesAlignedData",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"MeterValuesAlignedData"},
                            DeviceModel2_0 {{"AlignedDataCtrlr"}, {"Measurands"}, {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kMemberList,
                                std::nullopt,
                                std::nullopt,
                                "Energy.Active.Import.Register,Current.Import,Voltage"
                            }},
                            ""
                    };
                },
                detail::isValidMeasurandList
        };

        // TODO: Declare supported measurands?
        SettingString MeterValuesSampledData {
                []() {
                    return SettingMetadata {
                            "MeterValuesSampledData",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"MeterValuesSampledData"},
                            DeviceModel2_0 {{"SampledDataCtrlr"}, {"TxUpdatedMeasurands"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "Energy.Active.Import.Register,Current.Import,Voltage"
                            }},
                            "Energy.Active.Import.Register"
                    };
                },
                detail::isValidMeasurandList
        };

        SettingInt MeterValueSampleInterval {
                []() {
                    return SettingMetadata {
                            "MeterValueSampleInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"MeterValueSampleInterval"},
                            DeviceModel2_0 {{"SampledDataCtrlr"}, {"TxUpdatedInterval"}, {"s", ocpp2_0::DataEnumType::kinteger, 1, 86400}},
                            std::to_string(60)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt MinimumStatusDuration {
                []() {
                    return SettingMetadata {
                            "MinimumStatusDuration",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"MinimumStatusDuration"},
                            std::nullopt,
                            std::to_string(6)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt NumberOfConnectors {
                []() {
                    return SettingMetadata {
                            "NumberOfConnectors",
                            SettingConfig::roPolicy(),
                            DeviceModel1_6 {"NumberOfConnectors"},
                            std::nullopt,
                            std::to_string(1)
                    };
                },
                [](auto const& value) {return value >= 1;}
        };

        SettingInt ResetRetries {
                []() {
                    return SettingMetadata {
                            "ResetRetries",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"ResetRetries"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"ResetRetries"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingBool StopTransactionOnEVSideDisconnect {
                []() {
                    return SettingMetadata {
                            "StopTransactionOnEVSideDisconnect",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"StopTransactionOnEVSideDisconnect"},
                            DeviceModel2_0 {{"TxCtrlr"}, {"StopTxOnEVSideDisconnect"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool StopTransactionOnInvalidId {
                []() {
                    return SettingMetadata {
                            "StopTransactionOnInvalidId",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"StopTransactionOnInvalidId"},
                            DeviceModel2_0 {{"TxCtrlr"}, {"StopTxOnInvalidId"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString StopTxnAlignedData {
                []() {
                    return SettingMetadata {
                            "StopTxnAlignedData",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"StopTxnAlignedData"},
                            std::nullopt,
                            ""
                    };
                },
                [](auto const& value) {return value == "";}
        };

        SettingInt StopTxnAlignedDataMaxLength {
                []() {
                    return SettingMetadata {
                            "StopTxnAlignedDataMaxLength",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"StopTxnAlignedDataMaxLength"},
                            std::nullopt,
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingString StopTxnSampledData {
                []() {
                    return SettingMetadata {
                            "StopTxnSampledData",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"StopTxnSampledData"},
                            DeviceModel2_0 {{"SampledDataCtrlr"}, {"TxEndedMeasurands"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "Energy.Active.Import.Register,Current.Import,Voltage"
                            }},
                            ""
                    };
                },
                detail::isValidMeasurandList
        };

        SettingInt StopTxnSampledDataMaxLength {
                []() {
                    return SettingMetadata {
                            "StopTxnSampledDataMaxLength",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"StopTxnSampledDataMaxLength"},
                            std::nullopt,
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        // TODO: This has passed our tests. Should we advertise at least power management here?
        SettingString SupportedFeatureProfiles {
                []() {
                    return SettingMetadata {
                            "SupportedFeatureProfiles",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"SupportedFeatureProfiles"},
                            std::nullopt,
                            "Core"
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt TransactionMessageAttempts {
                []() {
                    return SettingMetadata {
                            "TransactionMessageAttempts",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"TransactionMessageAttempts"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"MessageAttempts", "TransactionEvent"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(4)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt TransactionMessageRetryInterval {
                []() {
                    return SettingMetadata {
                            "TransactionMessageRetryInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"TransactionMessageRetryInterval"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"MessageAttemptInterval", "TransactionEvent"}, {"s", ocpp2_0::DataEnumType::kinteger, 0, 600}},
                            std::to_string(10)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingBool UnlockConnectorOnEVSideDisconnect {
                []() {
                    return SettingMetadata {
                            "UnlockConnectorOnEVSideDisconnect",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"UnlockConnectorOnEVSideDisconnect"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"UnlockOnEVSideDisconnect"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt WebSocketPingInterval {
                []() {
                    return SettingMetadata {
                            "WebSocketPingInterval",
                            SettingConfig::rwPolicy(true),
                            DeviceModel1_6 {"WebSocketPingInterval"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"WebSocketPingInterval"}, {"s", ocpp2_0::DataEnumType::kinteger, 1, 86400}},
                            std::to_string(15)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt ChargeProfileMaxStackLevel {
                []() {
                    return SettingMetadata {
                            "ChargeProfileMaxStackLevel",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ChargeProfileMaxStackLevel"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"ProfileStackLevel"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(3)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt MaxChargingProfilesInstalled {
                []() {
                    return SettingMetadata {
                            "MaxChargingProfilesInstalled",
                            SettingConfig::roPolicy(),
                            DeviceModel1_6 {"MaxChargingProfilesInstalled"},
                            std::nullopt,
                            std::to_string(10)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        // Custom Extensions
        SettingBool BackendTransitionRequested {
                []() {
                    return SettingMetadata {
                            "BackendTransitionRequested",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"BackendTransitionRequested"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"BackendTransitionRequested"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingTransitionString ChargePointId {
                []() {
                    return SettingMetadata {
                            "ChargePointId",
                            SettingConfig::rwPolicy(true),
                            DeviceModel1_6 {"ChargePointId"},
                            DeviceModel2_0 {{"SecurityCtrlr"}, {"Identity"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                SettingTransitionType::kConnection,
                [](auto const& value) {
                    // Value must not exceed 48 characters and must not contain ':'
                    // Reference: OCPP 2.0.1 section 2.2.2.
                    if (value.size() > 48) {
                        CHARGELAB_LOG_MESSAGE(warning) << "ChargePointId exceeded max length of 48: " << value;
                        return false;
                    }

                    auto index = value.find(':');
                    if (index != std::string::npos) {
                        CHARGELAB_LOG_MESSAGE(warning) << "ChargePointId contained illegal ':' character: " << value;
                        return false;
                    }

                    return true;
                }
        };

        SettingInt MeterValuesMaxPointsPerRequest {
                []() {
                    return SettingMetadata {
                            "MeterValuesMaxPointsPerRequest",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"MeterValuesMaxPointsPerRequest"},
                            std::nullopt,
                            std::to_string(10)
                    };
                },
                [](auto const& value) {return value > 0;}
        };

        SettingTransitionString WifiSSID {
                []() {
                    return SettingMetadata {
                            "WifiSSID",
                            SettingConfig::rwPolicy(true),
                            DeviceModel1_6 {"WifiSSID"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"WifiSSID"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                SettingTransitionType::kConnection,
                [](auto const&) {return true;}
        };

        SettingTransitionString WifiPassword {
                []() {
                    return SettingMetadata {
                            "WifiPassword",
                            SettingConfig::woPolicy(true),
                            DeviceModel1_6 {"WifiPassword"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"WifiPassword"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                SettingTransitionType::kConnection,
                [](auto const&) {return true;}
        };

        SettingInt WifiReconnectInterval {
                []() {
                    return SettingMetadata {
                            "WifiReconnectInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"WifiReconnectInterval"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"WifiReconnectInterval"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(15)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool WifiHideStationAccessPointAfterSetup {
                []() {
                    return SettingMetadata {
                            "WifiHideStationAccessPointAfterSetup",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"WifiHideStationAccessPointAfterSetup"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"WifiHideStationAccessPointAfterSetup"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString PlugAndChargeId {
                []() {
                    return SettingMetadata {
                            "PlugAndChargeId",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"PlugAndChargeId"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"PlugAndChargeId"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool LogStreamingEnabled {
                []() {
                    return SettingMetadata {
                            "LogStreamingEnabled",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"LogStreamingEnabled"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"LogStreamingEnabled"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ActiveFirmwareSlotId {
                []() {
                    return SettingMetadata {
                            "ActiveFirmwareSlotId",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ActiveFirmwareSlotId"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ActiveFirmwareSlotId"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ExpectedUpdateFirmwareSlotId {
                []() {
                    return SettingMetadata {
                            "ExpectedUpdateFirmwareSlotId",
                            SettingConfig::roPolicy(),
                            DeviceModel1_6 {"ExpectedUpdateFirmwareSlotId"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ExpectedUpdateFirmwareSlotId"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool StopTransactionWithDifferentId {
                []() {
                    return SettingMetadata {
                            "StopTransactionWithDifferentId",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"StopTransactionWithDifferentId"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"StopTransactionWithDifferentId"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ConnectorAvailabilities {
                []() {
                    return SettingMetadata {
                            "ConnectorAvailabilities",
                            SettingConfig::roPolicy(),
                            DeviceModel1_6 {"ConnectorAvailabilities"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ConnectorAvailabilities"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            "[]"
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt DiagnosticFreeHeap {
                []() {
                    return SettingMetadata {
                            "DiagnosticFreeHeap",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DiagnosticFreeHeap"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ConnectorAvailabilities"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt DiagnosticFreeHeapMinimum {
                []() {
                    return SettingMetadata {
                            "DiagnosticFreeHeapMinimum",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DiagnosticFreeHeapMinimum"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ConnectorAvailabilities"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool OfflinePlugAndChargeToggle {
                []() {
                    return SettingMetadata {
                            "OfflinePlugAndChargeToggle",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"OfflinePlugAndChargeToggle"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"OfflinePlugAndChargeToggle"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool IgnoreChargingSchedulesWhileOffline {
                []() {
                    return SettingMetadata {
                            "IgnoreChargingSchedulesWhileOffline",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"IgnoreChargingSchedulesWhileOffline"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"IgnoreChargingSchedulesWhileOffline"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt DiagnosticWiFiSignalStrength {
                []() {
                    return SettingMetadata {
                            "DiagnosticWiFiSignalStrength",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DiagnosticWiFiSignalStrength"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"DiagnosticWiFiSignalStrength"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt DiagnosticLastPingRoundTripTime {
                []() {
                    return SettingMetadata {
                            "DiagnosticLastPingRoundTripTime",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DiagnosticLastPingRoundTripTime"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"DiagnosticLastPingRoundTripTime"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt RestartProtectionGracePeriodSeconds {
                []() {
                    return SettingMetadata {
                            "RestartProtectionGracePeriodSeconds",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"RestartProtectionGracePeriodSeconds"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"RestartProtectionGracePeriodSeconds"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(5 * 60 * 60)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt RestartProtectionGracePeriodAmperage {
                []() {
                    return SettingMetadata {
                            "RestartProtectionGracePeriodAmperage",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"RestartProtectionGracePeriodAmperage"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"RestartProtectionGracePeriodAmperage"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ConnectionTransitionTimeout {
                []() {
                    return SettingMetadata {
                            "ConnectionTransitionTimeout",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"ConnectionTransitionTimeout"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"ConnectionTransitionTimeout"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(5*60)
                    };
                },
                [](auto const& value) {return value > 0;}
        };

        SettingString InoperativeConnectors {
                []() {
                    return SettingMetadata {
                            "InoperativeConnectors",
                            SettingConfig::rwPolicy(true),
                            DeviceModel1_6 {"InoperativeConnectors"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InoperativeConnectors"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt InstalledV2GRootCertificateCount {
                []() {
                    return SettingMetadata {
                            "InstalledV2GRootCertificateCount",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"InstalledV2GRootCertificateCount"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InstalledCertificates", "V2GRootCertificate"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt InstalledMORootCertificateCount {
                []() {
                    return SettingMetadata {
                            "InstalledMORootCertificateCount",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"InstalledMORootCertificateCount"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InstalledCertificates", "MORootCertificate"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt InstalledCSMSRootCertificateCount {
                []() {
                    return SettingMetadata {
                            "InstalledCSMSRootCertificateCount",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"InstalledCSMSRootCertificateCount"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InstalledCertificates", "CSMSRootCertificate"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt InstalledV2GCertificateChainCount {
                []() {
                    return SettingMetadata {
                            "InstalledV2GCertificateChainCount",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"InstalledV2GCertificateChainCount"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InstalledCertificates", "V2GCertificateChain"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt InstalledManufacturerRootCertificateCount {
                []() {
                    return SettingMetadata {
                            "InstalledManufacturerRootCertificateCount",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"InstalledManufacturerRootCertificateCount"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"InstalledCertificates", "ManufacturerRootCertificate"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt FirmwareUpdateDefaultRetries {
                []() {
                    return SettingMetadata {
                            "FirmwareUpdateDefaultRetries",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"FirmwareUpdateDefaultRetries"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"FirmwareUpdateDefaultRetries"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(3)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt NotificationMessageDefaultRetries {
                []() {
                    return SettingMetadata {
                            "NotificationMessageDefaultRetries",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"NotificationMessageDefaultRetries"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"NotificationMessageDefaultRetries"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(2)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt NotificationMessageDefaultRetryInterval {
                []() {
                    return SettingMetadata {
                            "NotificationMessageDefaultRetryInterval",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"NotificationMessageDefaultRetries"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"NotificationMessageDefaultRetryInterval"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(15)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt MaxTransactionEndedMeterValuesBytes {
                []() {
                    return SettingMetadata {
                            "MaxTransactionEndedMeterValuesBytes",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"MaxTransactionEndedMeterValuesBytes"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"MaxTransactionEndedMeterValuesBytes"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(10*1024)
                    };
                },
                [](auto const&) {return true;}
        };

        // Internal settings
        SettingString ChargerVendor {
                []() {
                    return SettingMetadata {
                            "ChargerVendor",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerModel {
                []() {
                    return SettingMetadata {
                            "ChargerModel",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerSerialNumber {
                []() {
                    return SettingMetadata {
                            "ChargerSerialNumber",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerFirmwareVersion {
                []() {
                    return SettingMetadata {
                            "ChargerFirmwareVersion",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        // TODO: What was this and should it be exposed?
        SettingString ChargerAccessPointSSID {
                []() {
                    return SettingMetadata {
                            "ChargerAccessPointSSID",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerICCID {
                []() {
                    return SettingMetadata {
                            "ChargerICCID",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerIMSI {
                []() {
                    return SettingMetadata {
                            "ChargerIMSI",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerMeterSerialNumber {
                []() {
                    return SettingMetadata {
                            "ChargerMeterSerialNumber",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargerMeterType {
                []() {
                    return SettingMetadata {
                            "ChargerMeterType",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ConnectionProfiles {
                []() {
                    return SettingMetadata {
                            "ConnectionProfiles",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            std::nullopt,
                            "{1:{},2:{}}"
                    };
                },
                [](auto const&) {return true;}
        };

        // OCPP 2.0.1 settings
        SettingInt DefaultMessageTimeout {
                []() {
                    return SettingMetadata {
                            "DefaultMessageTimeout",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DefaultMessageTimeout"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"MessageTimeout", "Default"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(30)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingString FileTransferProtocols {
                []() {
                    return SettingMetadata {
                            "FileTransferProtocols",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"FileTransferProtocols"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"FileTransferProtocols"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "HTTP,HTTPS"
                            }},
                            "HTTP,HTTPS"
                    };
                },
                [](auto const&) {return true;}
        };

        SettingTransitionTypedDelimitedList<ocpp2_0::NetworkConnectionProfileType> NetworkConnectionProfiles {
                []() {
                    return SettingMetadata {
                            "NetworkConnectionProfiles",
                            SettingConfig::roPolicy(),
                            std::nullopt,
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"NetworkConnectionProfiles"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                SettingTransitionType::kConnection,
                "\n",
                (std::size_t)detail::SettingsConstants::kMaxProfileSlots,
                // TODO
                [](auto const&) {return true;}
        };

        SettingTransitionString NetworkConfigurationPriority {
                []() {
                    return SettingMetadata {
                            "NetworkConfigurationPriority",
                            SettingConfig::rwPolicy(true),
                            DeviceModel1_6 {"NetworkConfigurationPriority"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"NetworkConfigurationPriority"}, {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kSequenceList,
                                std::nullopt,
                                std::nullopt,
                                detail::rangeToSequenceList(0, detail::SettingsConstants::kMaxProfileSlots)
                            }},
                            "0"
                    };
                },
                SettingTransitionType::kConnection,
                // TODO
                [](auto const&) {return true;}
        };

        SettingInt ActiveNetworkProfile {
                []() {
                    return SettingMetadata {
                            "ActiveNetworkProfile",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"ActiveNetworkProfile"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt NetworkProfileConnectionAttempts {
                []() {
                    return SettingMetadata {
                            "NetworkProfileConnectionAttempts",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"NetworkProfileConnectionAttempts"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"NetworkProfileConnectionAttempts"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(5)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt NetworkConnectionRetryBackOffRepeatTimes {
                []() {
                    return SettingMetadata {
                            "NetworkConnectionRetryBackOffRepeatTimes",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"NetworkConnectionRetryBackOffRepeatTimes"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"RetryBackOffRepeatTimes"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt NetworkConnectionRetryBackOffRandomRange {
                []() {
                    return SettingMetadata {
                            "NetworkConnectionRetryBackOffRandomRange",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"NetworkConnectionRetryBackOffRandomRange"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"RetryBackOffRandomRange"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt NetworkConnectionRetryBackOffWaitMinimum {
                []() {
                    return SettingMetadata {
                            "NetworkConnectionRetryBackOffWaitMinimum",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"NetworkConnectionRetryBackOffWaitMinimum"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"RetryBackOffWaitMinimum"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(5)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt OfflineThreshold {
                []() {
                    return SettingMetadata {
                            "OfflineThreshold",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"OfflineThreshold"},
                            DeviceModel2_0 {{"OCPPCommCtrlr"}, {"OfflineThreshold"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(60)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingInt ItemsPerMessageGetReport {
                []() {
                    return SettingMetadata {
                            "ItemsPerMessageGetReport",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ItemsPerMessageGetReport"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"ItemsPerMessage", "GetReport"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(100)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ItemsPerMessageGetVariables {
                []() {
                    return SettingMetadata {
                            "ItemsPerMessageGetVariables",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ItemsPerMessageGetVariables"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"ItemsPerMessage", "GetVariables"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(100)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt BytesPerMessageGetReport {
                []() {
                    return SettingMetadata {
                            "BytesPerMessageGetReport",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"BytesPerMessageGetReport"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"BytesPerMessage", "GetReport"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(20000)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt BytesPerMessageGetVariables {
                []() {
                    return SettingMetadata {
                            "BytesPerMessageGetVariables",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"BytesPerMessageGetVariables"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"BytesPerMessage", "GetVariables"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(20000)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ItemsPerMessageSetVariables {
                []() {
                    return SettingMetadata {
                            "ItemsPerMessageSetVariables",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ItemsPerMessageSetVariables"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"ItemsPerMessage", "SetVariables"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(10)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt BytesPerMessageSetVariables {
                []() {
                    return SettingMetadata {
                            "BytesPerMessageSetVariables",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"BytesPerMessageSetVariables"},
                            DeviceModel2_0 {{"DeviceDataCtrlr"}, {"BytesPerMessage", "SetVariables"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(20000)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString DateTime {
                []() {
                    return SettingMetadata {
                            "DateTime",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DateTime"},
                            DeviceModel2_0 {{"ClockCtrlr"}, {"DateTime"}, {std::nullopt, ocpp2_0::DataEnumType::kdateTime}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString TimeSource {
                []() {
                    return SettingMetadata {
                            "TimeSource",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"TimeSource"},
                            DeviceModel2_0 {{"ClockCtrlr"}, {"TimeSource"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kSequenceList,
                                    std::nullopt,
                                    std::nullopt,
                                    "Heartbeat"
                            }},
                            "Heartbeat"
                    };
                },
                [](auto const& value) {return value == "Heartbeat";}
        };

        SettingTransitionString BasicAuthPassword {
                []() {
                    return SettingMetadata {
                            "BasicAuthPassword",
                            SettingConfig::woPolicy(true),
                            DeviceModel1_6 {"BasicAuthPassword"},
                            DeviceModel2_0 {{"SecurityCtrlr"}, {"BasicAuthPassword"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                SettingTransitionType::kConnection,
                [](auto const& value) {
                    // Note: requiring a minimum length of 16 as per 2.2.1. of the OCPP 2.0.1 specification
                    return value.empty() || value.size() >= 16;
                }
        };

        SettingString OrganizationName {
                []() {
                    return SettingMetadata {
                            "OrganizationName",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"OrganizationName"},
                            DeviceModel2_0 {{"SecurityCtrlr"}, {"OrganizationName"}, {std::nullopt, ocpp2_0::DataEnumType::kstring}},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt CertificateEntries {
                []() {
                    return SettingMetadata {
                            "CertificateEntries",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"CertificateEntries"},
                            DeviceModel2_0 {{"SecurityCtrlr"}, {"CertificateEntries"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger, 0, 10}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        // TODO: This needs to be part of a transition
        SettingInt SecurityProfile {
                []() {
                    return SettingMetadata {
                            "SecurityProfile",
                            SettingConfig::roPolicy(),
                            DeviceModel1_6 {"SecurityProfile"},
                            DeviceModel2_0 {{"SecurityCtrlr"}, {"SecurityProfile"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString TxStartPoint {
                []() {
                    return SettingMetadata {
                            "TxStartPoint",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"TxStartPoint"},
                            DeviceModel2_0 {{"TxCtrlr"}, {"TxStartPoint"}, ocpp2_0::VariableCharacteristicsType {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kMemberList,
                                std::nullopt,
                                std::nullopt,
                                "EVConnected,Authorized,PowerPathClosed"
                            }},
                            "PowerPathClosed"
                    };
                },
                [](auto const& text) {
                    bool valid = true;
                    string::SplitVisitor(text, ",", [&](std::string const& value) {
                        auto value_as_enum = ocpp2_0::TxStartPointValues::from_string(value);
                        switch (value_as_enum) {
                            // Valid
                            case ocpp2_0::TxStartPointValues::kEVConnected:
                            case ocpp2_0::TxStartPointValues::kAuthorized:
                            case ocpp2_0::TxStartPointValues::kPowerPathClosed:
                                break;

                            // Invalid - not supported
                            case ocpp2_0::TxStartPointValues::kValueNotFoundInEnum:
                            case ocpp2_0::TxStartPointValues::kParkingBayOccupancy:
                            case ocpp2_0::TxStartPointValues::kEnergyTransfer:
                            case ocpp2_0::TxStartPointValues::kDataSigned:
                                valid = false;
                                break;
                        }
                    });

                    return valid;
                }
        };

        SettingString TxStopPoint {
                []() {
                    return SettingMetadata {
                            "TxStopPoint",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"TxStopPoint"},
                            DeviceModel2_0 {{"TxCtrlr"}, {"TxStopPoint"}, ocpp2_0::VariableCharacteristicsType {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "EVConnected,Authorized,PowerPathClosed"
                            }},
                            "PowerPathClosed"
                    };
                },
                [](auto const& text) {
                    bool valid = true;
                    string::SplitVisitor(text, ",", [&](std::string const& value) {
                        auto value_as_enum = ocpp2_0::TxStopPointValues::from_string(value);
                        switch (value_as_enum) {
                            // Valid
                            case ocpp2_0::TxStopPointValues::kEVConnected:
                            case ocpp2_0::TxStopPointValues::kAuthorized:
                            case ocpp2_0::TxStopPointValues::kPowerPathClosed:
                                break;

                                // Invalid - not supported
                            case ocpp2_0::TxStopPointValues::kValueNotFoundInEnum:
                            case ocpp2_0::TxStopPointValues::kParkingBayOccupancy:
                            case ocpp2_0::TxStopPointValues::kEnergyTransfer:
                                valid = false;
                                break;
                        }
                    });

                    return valid;
                }
        };

        SettingInt SampledDataTxEndedInterval {
                []() {
                    return SettingMetadata {
                            "SampledDataTxEndedInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"SampledDataTxEndedInterval"},
                            DeviceModel2_0 {{"SampledDataCtrlr"}, {"TxEndedInterval"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingString SampledDataTxStartedMeasurands {
                []() {
                    return SettingMetadata {
                            "SampledDataTxStartedMeasurands",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"SampledDataTxStartedMeasurands"},
                            DeviceModel2_0 {{"SampledDataCtrlr"}, {"TxStartedMeasurands"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "Energy.Active.Import.Register,Current.Import,Voltage"
                            }},
                            "Energy.Active.Import.Register"
                    };
                },
                detail::isValidMeasurandList
        };

        SettingBool AlignedDataAvailable {
                []() {
                    return SettingMetadata {
                            "AlignedDataAvailable",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"AlignedDataAvailable"},
                            DeviceModel2_0 {{"AlignedDataCtrlr"}, {"Available"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString AlignedDataTxEndedMeasurands {
                []() {
                    return SettingMetadata {
                            "AlignedDataTxEndedMeasurands",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"AlignedDataTxEndedMeasurands"},
                            DeviceModel2_0 {{"AlignedDataCtrlr"}, {"TxEndedMeasurands"}, {
                                    std::nullopt,
                                    ocpp2_0::DataEnumType::kMemberList,
                                    std::nullopt,
                                    std::nullopt,
                                    "Energy.Active.Import.Register,Current.Import,Voltage"
                            }},
                            "Energy.Active.Import.Register"
                    };
                },
                detail::isValidMeasurandList
        };

        SettingInt AlignedDataTxEndedInterval {
                []() {
                    return SettingMetadata {
                            "AlignedDataTxEndedInterval",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"AlignedDataTxEndedInterval"},
                            DeviceModel2_0 {{"AlignedDataCtrlr"}, {"TxEndedInterval"}, {"s", ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const& value) {return value >= 0;}
        };

        SettingBool ReservationAvailable {
                []() {
                    return SettingMetadata {
                            "ReservationAvailable",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ReservationAvailable"},
                            DeviceModel2_0 {{"ReservationCtrlr"}, {"Available"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool SmartChargingAvailable {
                []() {
                    return SettingMetadata {
                            "SmartChargingAvailable",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"SmartChargingAvailable"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"Available"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString ChargingScheduleChargingRateUnit {
                []() {
                    return SettingMetadata {
                            "ChargingScheduleChargingRateUnit",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ChargingScheduleChargingRateUnit"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"RateUnit"}, {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kMemberList,
                                std::nullopt,
                                std::nullopt,
                                "A"
                            }},
                            "A"
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt PeriodsPerSchedule {
                []() {
                    return SettingMetadata {
                            "PeriodsPerSchedule",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"PeriodsPerSchedule"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"PeriodsPerSchedule"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(5)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ChargingProfileEntries {
                []() {
                    return SettingMetadata {
                            "ChargingProfileEntries",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ChargingProfileEntries"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"Entries", "ChargingProfiles"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool TariffAvailable {
                []() {
                    return SettingMetadata {
                            "TariffAvailable",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"TariffAvailable"},
                            DeviceModel2_0 {{"TariffCostCtrlr"}, {"Available", "Tariff"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ItemsPerMessageSetVariableMonitoring {
                []() {
                    return SettingMetadata {
                            "ItemsPerMessageSetVariableMonitoring",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ItemsPerMessageSetVariableMonitoring"},
                            DeviceModel2_0 {{"MonitoringCtrlr"}, {"ItemsPerMessage", "SetVariableMonitoring"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(10)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt BytesPerMessageSetVariableMonitoring {
                []() {
                    return SettingMetadata {
                            "BytesPerMessageSetVariableMonitoring",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"BytesPerMessageSetVariableMonitoring"},
                            DeviceModel2_0 {{"MonitoringCtrlr"}, {"BytesPerMessage", "SetVariableMonitoring"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(20000)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt NumberOfDisplayMessages {
                []() {
                    return SettingMetadata {
                            "NumberOfDisplayMessages",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"NumberOfDisplayMessages"},
                            DeviceModel2_0 {{"DisplayMessageCtrlr"}, {"DisplayMessages"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString DisplayMessageSupportedFormats {
                []() {
                    return SettingMetadata {
                            "DisplayMessageSupportedFormats",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DisplayMessageSupportedFormats"},
                            DeviceModel2_0 {{"DisplayMessageCtrlr"}, {"SupportedFormats"}, {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kMemberList,
                                std::nullopt,
                                std::nullopt,
                                "ASCII,HTML,URI,UTF8"
                            }},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingString DisplayMessageSupportedPriorities {
                []() {
                    return SettingMetadata {
                            "DisplayMessageSupportedPriorities",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"DisplayMessageSupportedPriorities"},
                            DeviceModel2_0 {{"DisplayMessageCtrlr"}, {"SupportedPriorities"}, {
                                std::nullopt,
                                ocpp2_0::DataEnumType::kMemberList,
                                std::nullopt,
                                std::nullopt,
                                "AlwaysFront,InFront,NormalCycle"
                            }},
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        SettingBool ContractValidationOffline {
                []() {
                    return SettingMetadata {
                            "ContractValidationOffline",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"ContractValidationOffline"},
                            DeviceModel2_0 {{"ISO15118Ctrlr"}, {"ContractValidationOffline"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextFalse
                    };
                },
                [](auto const&) {return true;}
        };

        SettingDouble LimitChangeSignificance {
                []() {
                    return SettingMetadata {
                            "LimitChangeSignificance",
                            SettingConfig::rwPolicy(),
                            DeviceModel1_6 {"LimitChangeSignificance"},
                            DeviceModel2_0 {{"SmartChargingCtrlr"}, {"LimitChangeSignificance"}, {std::nullopt, ocpp2_0::DataEnumType::kdecimal}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

//        SettingBool AuthCacheEnabled {
//                "AuthCacheEnabled",
//                SettingConfig::roNotSavedPolicy(),
//                DeviceModel1_6 {"AuthCacheEnabled"},
//                DeviceModel2_0 {{"AuthCacheCtrlr"}, {"Enabled"}},
//                false,
//                [](auto const&) {return true;}
//        };

        SettingString CustomBootReason {
                []() {
                    return SettingMetadata {
                            "CustomBootReason",
                            SettingConfig::rwPolicy(),
                            std::nullopt,
                            std::nullopt,
                            ""
                    };
                },
                [](auto const&) {return true;}
        };

        // Mandatory charging station level attributes
        SettingBool ChargingStationAvailable {
                []() {
                    return SettingMetadata {
                            "ChargingStationAvailable",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ChargingStationAvailable"},
                            DeviceModel2_0 {{"ChargingStation"}, {"Available"}, {std::nullopt, ocpp2_0::DataEnumType::kboolean}},
                            SettingBool::kTextTrue
                    };
                },
                [](auto const&) {return true;}

        };

        SettingString ChargingStationAvailabilityState {
                []() {
                    return SettingMetadata {
                            "ChargingStationAvailabilityState",
                            SettingConfig::roNotSavedPolicy(),
                            std::nullopt,
                            DeviceModel2_0 {
                                    ocpp2_0::ComponentType {"ChargingStation"},
                                    ocpp2_0::VariableType {"AvailabilityState"},
                                    ocpp2_0::VariableCharacteristicsType {
                                            std::nullopt,
                                            ocpp2_0::DataEnumType::kOptionList,
                                            std::nullopt,
                                            std::nullopt,
                                            "Available,Occupied,Reserved,Unavailable,Faulted"
                                    }},
                            "Available"
                    };
                },
                [](auto const&) {return true;}
        };

        SettingInt ChargingStationSupplyPhases {
                []() {
                    return SettingMetadata {
                            "ChargingStationSupplyPhases",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"ChargingStationSupplyPhases"},
                            DeviceModel2_0 {{"ChargingStation"}, {"SupplyPhases"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            "1"
                    };
                },
                [](auto const&) {return true;}
        };

        // Virtual settings
        SettingVirtual CentralSystemURL {
                []() {
                    return SettingMetadata {
                            "CentralSystemURL",
                            SettingConfig::rwNotSavedPolicy(),
                            DeviceModel1_6 {"CentralSystemURL"},
                            std::nullopt,
                            ""
                    };
                },
                this,
                [](Settings* settings) {
                    if (settings->NetworkConfigurationPriority.getValue() != "0")
                        return std::string {};

                    auto const& value = settings->NetworkConnectionProfiles.getValue(0);
                    if (!value.has_value())
                        return std::string {};

                    return value->ocppCsmsUrl.value();
                },
                [](Settings* settings, std::string const& value) {
                    if (!uri::ParseWebsocketUri(value).has_value())
                        return false;

                    auto profile = optional::GetOrDefault(
                            settings->NetworkConnectionProfiles.getValue(0),
                            ocpp2_0::NetworkConnectionProfileType {
                                ocpp2_0::OCPPVersionEnumType::kOCPP16,
                                ocpp2_0::OCPPTransportEnumType::kJSON,
                                value,
                                30,
                                0,
                                ocpp2_0::OCPPInterfaceEnumType::kWireless0,
                                std::nullopt,
                                std::nullopt
                            }
                    );

                    profile.ocppCsmsUrl = value;

                    if (!settings->NetworkConnectionProfiles.setValue(0, profile))
                        return false;
                    if (!settings->NetworkConfigurationPriority.setValue("0"))
                        return false;

                    return true;
                }
        };

        SettingVirtual OcppProtocol {
                []() {
                    return SettingMetadata {
                            "OcppProtocol",
                            SettingConfig::rwNotSavedPolicy(),
                            DeviceModel1_6 {"OcppProtocol"},
                            std::nullopt,
                            ""
                    };
                },
                this,
                [](Settings* settings) {
                    if (settings->NetworkConfigurationPriority.getValue() != "0")
                        return std::string {};

                    auto const& value = settings->NetworkConnectionProfiles.getValue(0);
                    if (!value.has_value())
                        return std::string {};

                    return write_json_to_string(value->ocppVersion);
                },
                [](Settings* settings, std::string const& value) {
                    auto const enum_value = ocpp2_0::OCPPVersionEnumType::from_string(value);
                    if (enum_value == ocpp2_0::OCPPVersionEnumType::kValueNotFoundInEnum) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Unrecognised OCPPVersionEnumType: " << value;
                        return false;
                    }

                    auto profile = optional::GetOrDefault(
                            settings->NetworkConnectionProfiles.getValue(0),
                            ocpp2_0::NetworkConnectionProfileType {
                                    enum_value,
                                    ocpp2_0::OCPPTransportEnumType::kJSON,
                                    "",
                                    30,
                                    0,
                                    ocpp2_0::OCPPInterfaceEnumType::kWireless0,
                                    std::nullopt,
                                    std::nullopt
                            }
                    );

                    profile.ocppVersion = enum_value;

                    if (!settings->NetworkConnectionProfiles.setValue(0, profile))
                        return false;
                    if (!settings->NetworkConfigurationPriority.setValue("0"))
                        return false;

                    return true;
                }
        };

        SettingInt MessageFlushCounter {
                []() {
                    return SettingMetadata {
                            "MessageFlushCounter",
                            SettingConfig::roNotSavedPolicy(),
                            DeviceModel1_6 {"MessageFlushCounter"},
                            DeviceModel2_0 {{"CustomizationCtrlr"}, {"MessageFlushCounter"}, {std::nullopt, ocpp2_0::DataEnumType::kinteger}},
                            std::to_string(0)
                    };
                },
                [](auto const&) {return true;}
        };

    public:
        // TODO: Shouldn't this be a unique pointer?
        Settings(std::shared_ptr<StorageInterface> storage_interface)
                : storage_interface_(std::move(storage_interface))
        {
            combined_setting_list_ = {
                    &ActiveFirmwareSlotId,
                    &ActiveNetworkProfile,
                    &AlignedDataAvailable,
                    &AlignedDataTxEndedInterval,
                    &AlignedDataTxEndedMeasurands,
                    &AllowOfflineTxForUnknownId,
                    &AuthorizeRemoteTxRequests,
                    &BackendTransitionRequested,
                    &BasicAuthPassword,
                    &BytesPerMessageGetReport,
                    &BytesPerMessageGetVariables,
                    &BytesPerMessageSetVariableMonitoring,
                    &BytesPerMessageSetVariables,
                    &CentralSystemURL,
                    &CertificateEntries,
                    &ChargePointId,
                    &ChargeProfileMaxStackLevel,
                    &ChargerAccessPointSSID,
                    &ChargerFirmwareVersion,
                    &ChargerICCID,
                    &ChargerIMSI,
                    &ChargerMeterSerialNumber,
                    &ChargerMeterType,
                    &ChargerModel,
                    &ChargerSerialNumber,
                    &ChargerVendor,
                    &ChargingProfileEntries,
                    &ChargingScheduleChargingRateUnit,
                    &ClockAlignedDataInterval,
                    &ConnectionProfiles,
                    &ConnectionTimeOut,
                    &ConnectionTransitionTimeout,
                    &ConnectorAvailabilities,
                    &ConnectorPhaseRotation,
                    &ContractValidationOffline,
                    &CustomBootReason,
                    &DateTime,
                    &DefaultMessageTimeout,
                    &DiagnosticFreeHeap,
                    &DiagnosticFreeHeapMinimum,
                    &DiagnosticLastPingRoundTripTime,
                    &DiagnosticWiFiSignalStrength,
                    &DisplayMessageSupportedFormats,
                    &DisplayMessageSupportedPriorities,
                    &ExpectedUpdateFirmwareSlotId,
                    &FileTransferProtocols,
                    &FirmwareUpdateDefaultRetries,
                    &GetConfigurationMaxKeys,
                    &HeartbeatInterval,
                    &IgnoreChargingSchedulesWhileOffline,
                    &InoperativeConnectors,
                    &InstalledCSMSRootCertificateCount,
                    &InstalledManufacturerRootCertificateCount,
                    &InstalledMORootCertificateCount,
                    &InstalledV2GCertificateChainCount,
                    &InstalledV2GRootCertificateCount,
                    &ItemsPerMessageGetReport,
                    &ItemsPerMessageGetVariables,
                    &ItemsPerMessageSetVariableMonitoring,
                    &ItemsPerMessageSetVariables,
                    &LimitChangeSignificance,
                    &LocalAuthorizeOffline,
                    &LocalPreAuthorize,
                    &LogStreamingEnabled,
                    &MaxChargingProfilesInstalled,
                    &MeterValuesAlignedData,
                    &MeterValueSampleInterval,
                    &MeterValuesMaxPointsPerRequest,
                    &MeterValuesSampledData,
                    &MinimumStatusDuration,
                    &NumberOfConnectors,
                    &NetworkConfigurationPriority,
                    &NetworkConnectionProfiles,
                    &NetworkConnectionRetryBackOffRandomRange,
                    &NetworkConnectionRetryBackOffRepeatTimes,
                    &NetworkConnectionRetryBackOffWaitMinimum,
                    &NetworkProfileConnectionAttempts,
                    &NotificationMessageDefaultRetries,
                    &NotificationMessageDefaultRetryInterval,
                    &NumberOfDisplayMessages,
                    &OcppProtocol,
                    &OfflinePlugAndChargeToggle,
                    &OfflineThreshold,
                    &OrganizationName,
                    &PeriodsPerSchedule,
                    &PlugAndChargeId,
                    &ReservationAvailable,
                    &ResetRetries,
                    &RestartProtectionGracePeriodAmperage,
                    &RestartProtectionGracePeriodSeconds,
                    &SampledDataTxEndedInterval,
                    &SampledDataTxStartedMeasurands,
                    &SecurityProfile,
                    &SmartChargingAvailable,
                    &StopTransactionOnEVSideDisconnect,
                    &StopTransactionOnInvalidId,
                    &StopTransactionWithDifferentId,
                    &StopTxnAlignedData,
                    &StopTxnAlignedDataMaxLength,
                    &StopTxnSampledData,
                    &StopTxnSampledDataMaxLength,
                    &SupportedFeatureProfiles,
                    &TariffAvailable,
                    &TimeSource,
                    &TransactionMessageAttempts,
                    &TransactionMessageRetryInterval,
                    &TxStartPoint,
                    &TxStopPoint,
                    &UnlockConnectorOnEVSideDisconnect,
                    &WebSocketPingInterval,
                    &WifiHideStationAccessPointAfterSetup,
                    &WifiPassword,
                    &WifiReconnectInterval,
                    &WifiSSID,
                    &ChargingStationAvailable,
                    &ChargingStationAvailabilityState,
                    &ChargingStationSupplyPhases,
                    &MessageFlushCounter
            };
        }

        virtual ~Settings() {
        }

        virtual void loadFromStorage() {
            std::lock_guard lock(mutex_);
            loadFromStorageInternal(std::nullopt);
        }

        virtual void saveIfModified() {
            std::lock_guard lock(mutex_);

            bool modified = false;
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;
                if (!p->getConfig().isIncludeInSave())
                    continue;
                if (!p->getModified())
                    continue;

                CHARGELAB_LOG_MESSAGE(info) << "Setting was modified: " << p->getId();
                modified = true;
            }

            if (!modified)
                return;

            CHARGELAB_LOG_MESSAGE(info) << "Setting modified - writing to file";
            CHARGELAB_TRY {
                storage_interface_->write([&](auto file) {
                    for (auto const& p : combined_setting_list_) {
                        if (p == nullptr)
                            continue;
                        if (!p->getConfig().isIncludeInSave())
                            continue;

                        file::json_write_object_to_file(file, detail::SettingKeyValue{
                                p->getId(),
                                p->save()
                        });
                    }

                    return true;
                });
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Failed writing settings: " << e.what();
            }
        }

        void registerCustomSetting(std::shared_ptr<SettingBase> setting) {
            if (setting == nullptr)
                return;

            std::lock_guard lock(mutex_);
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;
                if (!string::EqualsIgnoreCaseAscii(p->getId(), setting->getId()))
                    continue;

                CHARGELAB_LOG_MESSAGE(error) << "Custom setting name conflicts with existing entries: " << setting->getId();
                break;
            }

            auto const id = setting->getId();
            bool saved = setting->getMetadata().config.isIncludeInSave();
            combined_setting_list_.resize(combined_setting_list_.size()+1);
            combined_setting_list_.back() = setting.get();
            custom_setting_list_.resize(custom_setting_list_.size()+1);
            custom_setting_list_.back() = std::move(setting);

            if (saved) {
                CHARGELAB_LOG_MESSAGE(info) << "Loading from storage - new custom setting added: " << id;
                loadFromStorageInternal(id);
            }
        }

        [[nodiscard]] std::shared_ptr<SettingBase> getCustomSetting(std::string const& id) {
            std::lock_guard lock(mutex_);

            for (auto const& x : custom_setting_list_)
                if (string::EqualsIgnoreCaseAscii(id, x->getId()))
                    return x;

            return nullptr;
        }

        std::optional<SettingState> getSettingState(std::string const& id) {
            std::lock_guard lock(mutex_);
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;
                if (!string::EqualsIgnoreCaseAscii(p->getId(), id))
                    continue;
                
                return SettingState{
                        p->getId(),
                        p->getConfig(),
                        p->getModel16(),
                        p->getModel20(),
                        p->getValueAsString()
                };
            }

            return std::nullopt;
        }

        bool setSettingValue(std::string const& id, std::string const& value) {
            std::lock_guard lock(mutex_);
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;
                if (!string::EqualsIgnoreCaseAscii(p->getId(), id))
                    continue;
                
                return p->setValueFromString(value);
            }

            return false;
        }

        void visitSettings(std::function<void(SettingBase const&)> const& visitor) const {
            std::lock_guard lock(mutex_);
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;

                visitor(*p);
            }
        }

        void visitSettings(std::function<void(SettingBase&)> const& visitor) {
            std::lock_guard lock(mutex_);
            for (auto const& p : combined_setting_list_) {
                if (p == nullptr)
                    continue;

                visitor(*p);
            }
        }

        bool hasPendingTransition(SettingTransitionType const& type) {
            for (auto ptr : combined_setting_list_) {
                if (ptr != nullptr && ptr->hasPendingTransition(type)) {
                    CHARGELAB_LOG_MESSAGE(info) << "Setting was pending for: " << ptr->getId();
                    return true;
                }
            }

            return false;
        }

        bool hasRunningTransition(SettingTransitionType const& type) {
            std::lock_guard lock{mutex_};
            return active_transition_ == std::make_optional(type);
        }

        bool startTransition(SettingTransitionType const& type) {
            std::lock_guard lock{mutex_};
            if (active_transition_.has_value())
                return false;

            for (auto ptr : combined_setting_list_)
                if (ptr != nullptr && !ptr->startTransition(type))
                    return false;

            CHARGELAB_LOG_MESSAGE(info) << "Starting transition: " << type;
            active_transition_ = type;
            return true;
        }

        void commit(SettingTransitionType const& type) {
            std::lock_guard lock{mutex_};
            if (active_transition_ != type) {
                CHARGELAB_LOG_MESSAGE(warning) << "Commit outside of active transition: " << type;
                return;
            }

            CHARGELAB_LOG_MESSAGE(info) << "Committing transition: " << type;
            for (auto ptr : combined_setting_list_)
                if (ptr != nullptr)
                    ptr->commit(type);

            active_transition_ = std::nullopt;
        }

        void rollback(SettingTransitionType const& type) {
            std::lock_guard lock{mutex_};
            if (active_transition_ != type) {
                CHARGELAB_LOG_MESSAGE(warning) << "Rollback outside of active transition: " << type;
                return;
            }

            CHARGELAB_LOG_MESSAGE(info) << "Rolling back transition: " << type;
            for (auto ptr : combined_setting_list_)
                if (ptr != nullptr)
                    ptr->rollback(type);

            active_transition_ = std::nullopt;
        }

    private:
        void loadFromStorageInternal(std::optional<std::string> const& key) {
            CHARGELAB_TRY {
                storage_interface_->read([&](auto file) {
                    while (true) {
                        if (file::is_eof_ignore_whitespace(file))
                            break;

                        auto const json = file::json_read_object_from_file<detail::SettingKeyValue>(file);
                        if (!json.has_value())
                            continue;

                        auto const& record = json.value();
                        if (key.has_value() && record.key != key)
                            continue;

                        bool loaded_setting = false;
                        for (auto const& p : combined_setting_list_) {
                            if (p == nullptr)
                                continue;
                            if (!string::EqualsIgnoreCaseAscii(p->getId(), record.key))
                                continue;

                            if (p->load(record.value))
                                p->setModified(false);

                            CHARGELAB_LOG_MESSAGE(info) << "Load settings: " << record;
                            loaded_setting = true;
                            break;
                        }

                        if (!loaded_setting) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Unrecognised entry in saved settings: " << record;
                        }
                    }

                    return true;
                });
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Failed reading settings: " << e.what();
            }
        }

    private:
        std::shared_ptr<StorageInterface> storage_interface_;

        mutable std::recursive_mutex mutex_ {};
        std::vector<std::shared_ptr<SettingBase>> custom_setting_list_;
        std::vector<SettingBase*> combined_setting_list_;
        std::optional<SettingTransitionType> active_transition_;
    };

#undef CHARGELAB_DEVICE_MODEL
#undef CHARGELAB_MERGE_NAMES
#undef CHARGELAB_CREATE_CONFIG
#undef CHARGELAB_VISIT_CONFIG
}

#endif //CHARGELAB_OPEN_FIRMWARE_SETTINGS_H
