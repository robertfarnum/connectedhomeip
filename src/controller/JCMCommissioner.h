/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

 #pragma once

#include "JCMTrustVerification.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/DeviceProxy.h>
#include <app/ReadClient.h>
#include <controller/AutoCommissioner.h>
#include <controller/CHIPDeviceController.h>
#include <controller/JCMTrustVerification.h>
#include <lib/core/CHIPCallback.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPError.h>
#if CONFIG_DEVICE_LAYER
#include <platform/CHIPDeviceLayer.h>
#endif

namespace chip {

namespace Controller {

/*
* JCMCommissioner is a class that implements the internal Joint Commissioning Management (JCM) process
* for commissioning Joint Fabric Administrator devices in a CHIP network. It contains the implementation
* of the JCM trust verification process and is used by the JCMDeviceCommissioner class.
*/
struct JCMCommissioner
{
    // Constructor and destructor
    JCMCommissioner(JCMDeviceCommissioner & deviceCommissioner) : mDeviceCommissioner(deviceCommissioner){
        ChipLogProgress(Controller, "JCMCommissioner created");
    };
    ~JCMCommissioner(){
        ChipLogProgress(Controller, "JCMCommissioner destroyed");
        Cleanup();
    };

    CHIP_ERROR StartJCMTrustVerification(DeviceProxy * proxy);
    void OnJCMTrustVerificationComplete(const JCMTrustVerificationInfo  *info, JCMTrustVerificationResult result);
    void ContinueAfterUserConsent(bool consent);
    void Cleanup();

    // Parses the JCM administrator info from the commissioning info
    CHIP_ERROR ParseAdministratorInfo(ReadCommissioningInfo & info);
    CHIP_ERROR ParseAdminFabricIndexAndEndpointId(ReadCommissioningInfo & info);
    CHIP_ERROR ParseOperationalCredentials(ReadCommissioningInfo & info);
    CHIP_ERROR ParseTrustedRoot(ReadCommissioningInfo & info);

    // Move to next JCM commissioning step
    void AdvanceTrustVerificationStage(JCMTrustVerificationResult result);

    // JCM commissioning trust verification steps
    void VerifyAdministratorEndpointAndFabricIndex();
    void PerformVendorIDVerificationProcedure();
    void VerifyNOCContainsAdministratorCAT();
    void AskUserForConsent();
    void setTrustVerificationDelegate(std::shared_ptr<JCMTrustVerificationDelegate> trustVerificationDelegate)
    {
        ChipLogProgress(Controller, "JCM: Setting trust verification delegate");
        mTrustVerificationDelegate = trustVerificationDelegate;
    }

    JCMTrustVerificationStage GetNextStage() const
    {
        return mNextStage;
    }

    JCMTrustVerificationInfo & GetJCMTrustVerificationInfo()
    {
        return mInfo;
    }

private:
    // Initial stage
    JCMTrustVerificationStage mNextStage = JCMTrustVerificationStage::kIdle;

    // Trust verification delegate for the commissioning client
    std::shared_ptr<JCMTrustVerificationDelegate> mTrustVerificationDelegate = nullptr;

    // Device proxy for the device being commissioned
    DeviceProxy * mDeviceProxy = nullptr;

    // JCM trust verification info
    // This structure contains the information needed for JCM trust verification
    // such as the administrator fabric index, endpoint ID, and vendor ID
    // It is used to store the results of the trust verification process
    // and is passed to the JCM trust verification delegate
    // when the trust verification process is complete
    JCMTrustVerificationInfo mInfo;

    // Reference to the JCMDeviceCommissioner that owns this commissioner
    JCMDeviceCommissioner & mDeviceCommissioner;
};

/*
 * JCMDeviceCommissioner is a class that handles the Joint Commissioning Management (JCM) process
 * for commissioning Joint Fabric Administrator devices in a CHIP network. It extends the DeviceCommissioner class and
 * implements the JCM trust verification process.
 */
class JCMDeviceCommissioner : public DeviceCommissioner
{
public:
    // The constructor initializes the JCMCommissioner with a reference to this device commissioner
    JCMDeviceCommissioner() : mCommissioner(*this) {
        ChipLogProgress(Controller, "JCMDeviceCommissioner created");
    }
    ~JCMDeviceCommissioner() {
        ChipLogProgress(Controller, "JCMDeviceCommissioner destroyed");
    };

    // Builder class to create a JCMDeviceCommissioner instance
    class Builder {
    public:
        Builder & setTrustVerificationDelegate(std::shared_ptr<JCMTrustVerificationDelegate> delegate)
        {
            mTrustVerificationDelegate = delegate;

            return *this;
        }

        // Factory method to create a JCMDeviceCommissioner instance
        std::unique_ptr<JCMDeviceCommissioner> Build()
        {
            std::unique_ptr<JCMDeviceCommissioner> deviceCommissioner = std::make_unique<JCMDeviceCommissioner>();
            deviceCommissioner->GetCommissioner().setTrustVerificationDelegate(mTrustVerificationDelegate);

            return deviceCommissioner;
        }

    private:
        std::shared_ptr<JCMTrustVerificationDelegate> mTrustVerificationDelegate = nullptr;
    };

    // Static method to get the Builder instance
    static Builder builder() {
        return Builder();
    }

    JCMCommissioner & GetCommissioner()
    {
        return mCommissioner;
    }

    /*
     * StartJCMTrustVerification is a method that initiates the JCM trust verification process for the device.
     * It is called by the commissioning client to start the trust verification process.
     * The method will return an error if the device proxy is null or if the trust verification process fails.
     * 
     * @param proxy The DeviceProxy for the device being commissioned.
     * @return CHIP_ERROR indicating success or failure of the operation.
     */
    CHIP_ERROR StartJCMTrustVerification(DeviceProxy * proxy) override;
    
    /*
     * ContinueAfterUserConsent is a method that continues the JCM trust verification process after the user has
     * provided consent or denied it. If the user grants consent, the trust verification process will continue;
     * otherwise, it will terminate with an error.
     * 
     * @param consent A boolean indicating whether the user granted consent (true) or denied it (false).
     */
    void ContinueAfterUserConsent(bool consent);

    /*
     * JCMTrustVerificationStageToString is a utility function that converts a JCMTrustVerificationStage enum value
     * to its string representation for logging purposes.
     * 
     * @param stage The JCMTrustVerificationStage to convert.
     * @return A string representation of the JCMTrustVerificationStage.
     */
    std::string JCMTrustVerificationStageToString(JCMTrustVerificationStage stage) {
        switch (stage) {
            case kIdle: return "IDLE";
            case kStarted: return "STARTED";
            case kVerifyingAdministratorEndpointAndFabricIndex: return "VERIFYING_ADMINISTRATOR_ENDPOINT_AND_FABRIC_INDEX";
            case kPerformingVendorIDVerificationProcedure: return "PERFORMING_VENDOR_ID_VERIFICATION_PROCEDURE";
            case kVerifyingNOCContainsAdministratorCAT: return "VERIFYING_NOC_CONTAINS_ADMINISTRATOR_CAT";
            case kAskingUserForConsent: return "ASKING_USER_FOR_CONSENT";
            default: return "UNKNOWN";
        }
    }

protected:
    // Override FinishReadingCommissioningInfo to parse JCM administrator info
    void FinishReadingCommissioningInfo(CHIP_ERROR & err, ReadCommissioningInfo & info) override;
    // Override CleanupCommissioning to clean up JCM trust verification state
    void CleanupCommissioning(DeviceProxy * proxy, NodeId nodeId, const CompletionStatus & completionStatus) override;

private:
    // Pointer to the JCMCommissioner that owns this device commissioner;
    JCMCommissioner mCommissioner;
};

/*
 * JCMAutoCommissioner is a class that handles the Joint Commissioning Management (JCM) process
 * for commissioning Joint Fabric Administrator devices in a CHIP network. It extends the AutoCommissioner class and
 * helps setup for the JCM trust verification process.
 */
class JCMAutoCommissioner : public AutoCommissioner
{
public:
    JCMAutoCommissioner(){};
    ~JCMAutoCommissioner(){};

    CHIP_ERROR SetCommissioningParameters(const CommissioningParameters & params) override;
    void CleanupCommissioning() override;

private:
     // Joint Fabric Management: all attributes
     const std::vector<app::AttributePathParams> mExtraReadPaths =
     {
         app::AttributePathParams(app::Clusters::JointFabricAdministrator::Id, app::Clusters::JointFabricAdministrator::Attributes::AdministratorFabricIndex::Id),
         app::AttributePathParams(kRootEndpointId, app::Clusters::OperationalCredentials::Id, app::Clusters::OperationalCredentials::Attributes::Fabrics::Id),
         app::AttributePathParams(kRootEndpointId, app::Clusters::OperationalCredentials::Id, app::Clusters::OperationalCredentials::Attributes::NOCs::Id),
         app::AttributePathParams(kRootEndpointId, app::Clusters::OperationalCredentials::Id, app::Clusters::OperationalCredentials::Attributes::TrustedRootCertificates::Id)
     };
    std::vector<app::AttributePathParams> mTempReadPaths;
};

} // namespace Controller
} // namespace chip