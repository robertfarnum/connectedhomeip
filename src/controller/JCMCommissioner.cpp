/*
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

#include "JCMCommissioner.h"
#include "JCMTrustVerification.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/InteractionModelEngine.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPError.h>
#include <controller/JCMCommissioner.h>

using namespace ::chip;
using namespace ::chip::app;
using namespace chip::app::Clusters;

namespace chip {
namespace Controller {

CHIP_ERROR JCMCommissioner::StartJCMTrustVerification(DeviceProxy * proxy)
{
    ChipLogProgress(Controller, "Starting JCM Trust Verification");

    VerifyOrReturnError(proxy != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    mNextStage = JCMTrustVerificationStage::kStarted;
    mDeviceProxy = proxy;
    mAttributeCache = Platform::MakeUnique<chip::app::ClusterStateCache>(*this);

    AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
    
    return CHIP_NO_ERROR;  
}

void JCMCommissioner::OnJCMTrustVerificationComplete(JCMTrustVerificationInfo *info, JCMTrustVerificationResult result)
{
    ChipLogProgress(Controller, "Administrator Device passed JCM Trust Verification");

    if (result == JCMTrustVerificationResult::kSuccess)
    {
        CommissioningStageComplete(CHIP_NO_ERROR);
    }
    else
    {
        ChipLogError(Controller, "Failed in verifying 'JCM Trust Verification': err %hu",
                     static_cast<uint16_t>(result));
        CommissioningDelegate::CommissioningReport report;
        report.Set<JCMTrustVerificationError>(result);
        CommissioningStageComplete(CHIP_ERROR_INTERNAL, report);
    }
}

void JCMCommissioner::DiscoverAdministratorEndpoint()
{
    ChipLogProgress(Controller, "Discovering Joint Fabric Administrator endpoint");

    Optional<System::Clock::Timeout> mTimeout;

    // Create attribute path for ServerList on all endpoints
    AttributePathParams attributePath;
    attributePath.mEndpointId = 0xFFFFU;
    attributePath.mClusterId = chip::app::Clusters::Descriptor::Id;
    attributePath.mAttributeId = chip::app::Clusters::Descriptor::Attributes::DeviceTypeList::Id;
    
    SendCommissioningReadRequest(mDeviceProxy, mTimeout, &attributePath, 1);
}

void JCMCommissioner::ReadAdministratorFabricIndex()
{
    ChipLogProgress(Controller, "Reading Administrator Fabric Index");

    // TODO: Implement the read request for Administrator Fabric Index

    AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
}

void JCMCommissioner::PerformVendorIDVerificationProcedure()
{
    ChipLogProgress(Controller, "Performing Vendor ID Verification Procedure");

    // TODO: Implement the Vendor ID verification procedure

    AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
}

void JCMCommissioner::VerifyNOCContainsAdministratorCAT()
{
    ChipLogProgress(Controller, "Verifying NOC contains Administrator CAT");

    // TODO: Implement the verification of NOC containing Administrator CAT

    AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
}

void JCMCommissioner::FindAdministratorEndpoint() {
    ChipLogProgress(Controller, "Searching for the Administrator Endpoint in the ServerList");

    // TODO: Parse the mAttributeCache device-type-list here
   
    AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
}

void JCMCommissioner::AskUserForConsent()
{
    ChipLogProgress(Controller, "Asking user for consent");
    if (mJCMTrustVerificationDelegate != nullptr)
    {
        VendorId vendorId = static_cast<VendorId>(0xFFFFU); // TODO: Set the vendor ID to the appropriate value
        mJCMTrustVerificationDelegate->OnAskUserForConsent(this, vendorId);
    } else {
        ChipLogError(Controller, "JCMTrustVerificationDelegate is not set");
        AdvanceTrustVerificationStage(JCMTrustVerificationResult::kTrustVerificationDelegateNotSet);
    }
}

void JCMCommissioner::ContinueAfterUserConsent(bool consent)
{
    if (consent)
    {
        ChipLogProgress(Controller, "User consent granted");
        AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);
    }
    else
    {
        ChipLogError(Controller, "User denied consent");
        AdvanceTrustVerificationStage(JCMTrustVerificationResult::KUserDeniedConsent);
    }
}

void JCMCommissioner::OnDone(chip::app::ReadClient * readClient)
{
    ChipLogProgress(Controller, "JCMCommissioner::OnDone called for read client");
    // Check if the read client is valid
    VerifyOrDie(readClient != nullptr && readClient == mReadClient.get());

    switch (mNextStage)
    {
        case JCMTrustVerificationStage::kDiscoveringAdministratorEndpoint:
            mReadClient.reset();
            FindAdministratorEndpoint();
            break;
        default:
            DeviceCommissioner::OnDone(readClient);
    }
}

void JCMCommissioner::AdvanceTrustVerificationStage(JCMTrustVerificationResult result)
{
    if (mJCMTrustVerificationDelegate != nullptr)
    {
        mJCMTrustVerificationDelegate->OnProgressUpdate(this, mNextStage, result);
    }
    
    if (result != JCMTrustVerificationResult::kSuccess)
    {
        // Handle error
        ChipLogError(Controller, "Error in Joint Commissioning Trust Verification: %d", static_cast<int>(result));
        OnJCMTrustVerificationComplete(nullptr, result);
        return;
    }

    switch (mNextStage)
    {
        case chip::Controller::JCMTrustVerificationStage::kStarted:
            mNextStage = JCMTrustVerificationStage::kDiscoveringAdministratorEndpoint;
            DiscoverAdministratorEndpoint();
            break;
        case JCMTrustVerificationStage::kDiscoveringAdministratorEndpoint:
            mNextStage = JCMTrustVerificationStage::kReadingAdministratorFabricIndex;
            ReadAdministratorFabricIndex();
            break;
        case JCMTrustVerificationStage::kReadingAdministratorFabricIndex:
            mNextStage = JCMTrustVerificationStage::kPerformingVendorIDVerificationProcedure;
            PerformVendorIDVerificationProcedure();
            break;
        case JCMTrustVerificationStage::kPerformingVendorIDVerificationProcedure:
            mNextStage = JCMTrustVerificationStage::kVerifyingNOCContainsAdministratorCAT;
            VerifyNOCContainsAdministratorCAT();
            break;
        case JCMTrustVerificationStage::kVerifyingNOCContainsAdministratorCAT:
            mNextStage = JCMTrustVerificationStage::kAskingUserForConsent;
            AskUserForConsent();
            break;
        case JCMTrustVerificationStage::kAskingUserForConsent:
            // Handle the response for user consent
            ChipLogProgress(Controller, "Joint Commissioning Trust Verification completed successfully");
            mNextStage = JCMTrustVerificationStage::kIdle;
            JCMTrustVerificationInfo info;
            info.endPointId = 0xFFFFU; // TODO: Set the endpoint ID to the appropriate value
            OnJCMTrustVerificationComplete(&info, result);
            break;
        default:
            ChipLogError(Controller, "Invalid stage: %d", static_cast<int>(mNextStage));
            OnJCMTrustVerificationComplete(nullptr, JCMTrustVerificationResult::kInternalError);
            break;
    }
}

} // namespace Controller
} // namespace chip
