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

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/DeviceProxy.h>
#include <app/ReadClient.h>
#include <controller/AutoCommissioner.h>
#include <controller/CHIPDeviceController.h>
#include <controller/JCMTrustVerification.h>
#include <lib/core/CHIPCallback.h>
#include <lib/core/CHIPError.h>
#if CONFIG_DEVICE_LAYER
#include <platform/CHIPDeviceLayer.h>
#endif

namespace chip {

namespace Controller {

class JCMAutoCommissioner : public AutoCommissioner
{
public:
    JCMAutoCommissioner(){};
    ~JCMAutoCommissioner(){};

    CHIP_ERROR SetCommissioningParameters(const CommissioningParameters & params) override;
    void CommissioningStepCleanup() override
    {
        AutoCommissioner::CommissioningStepCleanup();
        mTempReadPaths.clear();
    }

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

class JCMDeviceCommissioner : public DeviceCommissioner
{
public:
    JCMDeviceCommissioner(){};
    ~JCMDeviceCommissioner(){};

    CHIP_ERROR StartJCMTrustVerification(DeviceProxy * proxy) override;
    void OnJCMTrustVerificationComplete(const JCMTrustVerificationInfo  *info, JCMTrustVerificationResult result);
    void RegisterJCMTrustVerificationDelegate(JCMTrustVerificationDelegate * jcmTrustVerificationDelegate)
    {
        mJCMTrustVerificationDelegate = jcmTrustVerificationDelegate;
    }
    void ContinueAfterUserConsent(bool consent);

    // Convert JCMTrustVerificationStage to string for logging
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
    void CommissioningStepCleanup() override
    {
        DeviceCommissioner::CommissioningStepCleanup();
        mNextStage = JCMTrustVerificationStage::kIdle;
        mJCMTrustVerificationDelegate = nullptr;
        mInfo.clear();
    }


private:    
    // Move to next JCM commissioning step
    void AdvanceTrustVerificationStage(JCMTrustVerificationResult result);

    // Parse the JCM administrator info from the commissioning info
    CHIP_ERROR ParseAdministratorInfo(ReadCommissioningInfo & info);
    CHIP_ERROR FindAdminFabricIndexAndEndpointId();
    CHIP_ERROR GetOperationalCredentials();
    CHIP_ERROR GetTrustedRoot();

    // JCM commissioning trust verification steps
    void VerifyAdministratorEndpointAndFabricIndex();
    void PerformVendorIDVerificationProcedure();
    void VerifyNOCContainsAdministratorCAT();
    void AskUserForConsent();

    // Initial stage
    JCMTrustVerificationStage mNextStage = JCMTrustVerificationStage::kIdle;

    // Trust verification delegate for the commissioning client
    JCMTrustVerificationDelegate * mJCMTrustVerificationDelegate = nullptr;
    DeviceProxy * mDeviceProxy = nullptr;
    JCMTrustVerificationInfo mInfo;
};

} // namespace Controller
} // namespace chip