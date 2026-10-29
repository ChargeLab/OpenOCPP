#pragma once

#include <functional>
#include <optional>

#include "openocpp/protocol/ocpp2_0/messages/get_variables.h"
#include "openocpp/protocol/ocpp2_0/messages/set_variables.h"
#include "openocpp/protocol/ocpp2_0/types/report_data_type.h"

namespace chargelab {

    /**
     * Optional interface for exposing the device model of another charger through OCPP 2.0.1 GetVariables,
     * SetVariables and GetBaseReport.
     *
     * ConfigurationModule answers for the variables of OpenOCPP's own settings. This is used when OpenOCPP represents
     * another charger, which has variables of its own: the controller answers for those OpenOCPP has no setting for.
     * The variables OpenOCPP has are always its own; the controller is told about changes of those, so it can pass them
     * on to the other charger.
     */
    class ConfigurationController2_0 {
    public:
        virtual ~ConfigurationController2_0() = default;

        /**
         * Called for each variable of a GetVariables request that OpenOCPP has no setting for.
         *
         * @return the result to answer with, or nullopt if the variable is not known (OpenOCPP answers UnknownComponent
         *         or UnknownVariable).
         */
        virtual std::optional<ocpp2_0::GetVariableResultType> onGetVariable(
                const ocpp2_0::GetVariableDataType& data) = 0;

        /**
         * Called for each variable of a SetVariables request that OpenOCPP has no setting for.
         *
         * @return the status to answer with, or nullopt if the variable is not known (OpenOCPP answers UnknownComponent
         *         or UnknownVariable).
         */
        virtual std::optional<ocpp2_0::SetVariableStatusEnumType> onSetVariable(
                const ocpp2_0::SetVariableDataType& data) = 0;

        /**
         * Called after OpenOCPP changed one of its own settings for a SetVariables request, with the status it answered:
         * Accepted, or RebootRequired if the change takes effect after a restart.
         */
        virtual void onVariableChanged(
                const ocpp2_0::SetVariableDataType& data,
                ocpp2_0::SetVariableStatusEnumType status) {
        }

        /**
         * Visits the variables the controller answers for in a GetBaseReport (after those of OpenOCPP). Variables
         * OpenOCPP has a setting for are skipped.
         */
        virtual void visitVariables(std::function<void(ocpp2_0::ReportDataType const&)> const& visitor) = 0;
    };

}  // namespace chargelab
