#ifndef CHARGELAB_OPEN_FIRMWARE_CHARGING_PROFILE_PERSISTENCE_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_CHARGING_PROFILE_PERSISTENCE_INTERFACE_H

#include <string>
#include <vector>
#include <optional>
#include "openocpp/protocol/ocpp1_6/types/charging_profile.h"
#include "openocpp/helpers/json.h"

namespace chargelab {
    struct ActiveProfileIdsPersistenceInfo {
        std::unordered_set<int> activeRestartProtectionProfileIds;
        std::optional<SystemTimeMillis> lastUpdated;
    };

    CHARGELAB_OPTIONAL_NULL_DEFINE_TYPE_NON_INTRUSIVE(
            ActiveProfileIdsPersistenceInfo,
            activeRestartProtectionProfileIds,
            lastUpdated
    )

    struct ChargingProfilePersistenceInfo {
        std::vector<pair<int, ocpp1_6::ChargingProfile>> profiles;
        int criticalWriteCredits;
    };

    CHARGELAB_OPTIONAL_NULL_DEFINE_TYPE_NON_INTRUSIVE(
            ChargingProfilePersistenceInfo,
            profiles,
            criticalWriteCredits
    )

    class ChargingProfilePersistenceInterface {
    public:
        virtual ~ChargingProfilePersistenceInterface() = default;

        virtual std::optional<ChargingProfilePersistenceInfo> loadProfilePersistenceInfo() = 0;
        virtual bool saveProfilePersistenceInfo(ChargingProfilePersistenceInfo const & profiles) = 0;

        virtual std::optional<ActiveProfileIdsPersistenceInfo> loadActiveProfileIdsInfo() = 0;
        virtual bool saveActiveProfileIdsInfo(ActiveProfileIdsPersistenceInfo const & persistence_info) = 0;

        virtual bool saveWriteCredits(int write_credits) = 0;

        virtual int getCreditIncreaseIntervalSeconds() = 0;  // this is for making unit test easier
        virtual int getActiveIdsUpdateIntervalSeconds() = 0; // for making unit test easier
        virtual int getCheckProfileExpiryIntervalSeconds() = 0; // for making unit test easier
        virtual int getFlashCreditInterval() = 0; // how many difference between the memory value and disk value, for making unit test easier
        virtual int getFlashCreditMaximumLimit() = 0; // the biggest credit value allowed to write to disk, for making unit test easier
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_CHARGING_PROFILE_PERSISTENCE_INTERFACE_H
