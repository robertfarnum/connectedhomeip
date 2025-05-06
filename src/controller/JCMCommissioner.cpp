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
#include "CommissioningDelegate.h"
#include "JCMTrustVerification.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/InteractionModelEngine.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPError.h>
#include <controller/JCMCommissioner.h>
#include <credentials/CHIPCert.h>

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

void JCMCommissioner::OnJCMTrustVerificationComplete(const JCMTrustVerificationInfo  *info, JCMTrustVerificationResult result)
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

CHIP_ERROR JCMCommissioner::ParseAdministratorInfo(ReadCommissioningInfo & info)
{
    using namespace OperationalCredentials::Attributes;
    ByteSpan rootKeySpan;

    CHIP_ERROR err = CHIP_NO_ERROR;

    // err = mAttributeCache->ForEachAttribute(Clusters::JointFabricAdministrator::Id, [this, &info](const ConcreteAttributePath & path) {
    //     using namespace Clusters::JointFabricAdministrator::Attributes;
    //     AdministratorFabricIndex::TypeInfo::DecodableType administratorFabricIndex;

    //     VerifyOrReturnError(path.mAttributeId == AdministratorFabricIndex::Id, CHIP_NO_ERROR);
    //     ReturnErrorOnFailure(this->mAttributeCache->Get<AdministratorFabricIndex::TypeInfo>(path, administratorFabricIndex));

    //     if (!administratorFabricIndex.IsNull())
    //     {
    //         ChipLogProgress(Controller, "JCM: AdministratorFabricIndex: %d", administratorFabricIndex.Value());
    //         mInfo.adminFabricIndex = administratorFabricIndex.Value();
    //         mInfo.adminEndpointId = path.mEndpointId;
    //     }
    //     else
    //     {
    //         ChipLogError(Controller, "JCM: AdministratorFabricIndex attribute@JF Administrator Cluster not found!");
    //         return CHIP_ERROR_NOT_FOUND;
    //     }
    //     return CHIP_NO_ERROR;
    // });

    if (err != CHIP_NO_ERROR)
    {
        return err;
    }

    err = mAttributeCache->ForEachAttribute(OperationalCredentials::Id, [this, &rootKeySpan](const ConcreteAttributePath & path) {
        using namespace chip::app::Clusters::OperationalCredentials::Attributes;

        switch (path.mAttributeId)
        {
            case Fabrics::Id: {
                Fabrics::TypeInfo::DecodableType fabrics;
                ReturnErrorOnFailure(this->mAttributeCache->Get<Fabrics::TypeInfo>(path, fabrics));

                auto iter = fabrics.begin();
                while (iter.Next())
                {
                    auto & fabricDescriptor = iter.GetValue();
                    if (fabricDescriptor.fabricIndex == mInfo.adminFabricIndex)
                    {
                        if (fabricDescriptor.rootPublicKey.size() != Crypto::kP256_PublicKey_Length)
                        {
                            ChipLogError(Controller, "JCM: DeviceCommissioner::ParseJFAdministratorInfo - fabric root key size mismatch");
                            return CHIP_ERROR_KEY_NOT_FOUND;
                        }
                        rootKeySpan = fabricDescriptor.rootPublicKey;
                        mInfo.adminVendorId = fabricDescriptor.vendorID;
                        mInfo.adminFabricId = fabricDescriptor.fabricID;

                        if (fabricDescriptor.VIDVerificationStatement.HasValue())
                        {
                            ChipLogError(Controller, "JCM: Per-home RCAC are not supported by JF for now!");
                            return CHIP_ERROR_CANCELLED;
                        }
                        ChipLogProgress(Controller, "JCM: Successfully parsed the Administrator Fabric Table");
                        break;
                    }
                }
                return CHIP_NO_ERROR;
            }
            case NOCs::Id: {
                NOCs::TypeInfo::DecodableType nocs;
                ReturnErrorOnFailure(this->mAttributeCache->Get<NOCs::TypeInfo>(path, nocs));

                auto iter = nocs.begin();
                while (iter.Next())
                {
                    auto & nocStruct = iter.GetValue();

                    if (nocStruct.fabricIndex == mInfo.adminFabricIndex)
                    {
                        mInfo.adminNOC = nocStruct.noc;

                        if (!nocStruct.icac.IsNull())
                        {
                            mInfo.adminICAC = nocStruct.icac.Value();
                        }
                        else
                        {
                            ChipLogError(Controller, "JCM: ICAC not present!");
                            return CHIP_ERROR_CERT_NOT_FOUND;
                        }
                        ChipLogProgress(Controller, "JCM: Successfully parsed the Administrator NOC and ICAC");
                        break;
                    }
                }
                return CHIP_NO_ERROR;
            }
            default:
                return CHIP_NO_ERROR;
        }

        return CHIP_NO_ERROR;
    });

    if (err != CHIP_NO_ERROR)
    {
        mInfo.adminFabricIndex = kUndefinedFabricIndex;
        return err;
    }

    err = mAttributeCache->ForEachAttribute(OperationalCredentials::Id, [this, &rootKeySpan](const ConcreteAttributePath & path) {
        using namespace chip::app::Clusters::OperationalCredentials::Attributes;
        bool foundMatchingRcac = false;

        switch (path.mAttributeId)
        {
            case TrustedRootCertificates::Id: {
                TrustedRootCertificates::TypeInfo::DecodableType trustedCAs;
                ReturnErrorOnFailure(this->mAttributeCache->Get<TrustedRootCertificates::TypeInfo>(path, trustedCAs));

                   auto iter = trustedCAs.begin();
                   while (iter.Next())
                   {
                       auto & trustedCA = iter.GetValue();
                       Credentials::P256PublicKeySpan trustedCAPublicKeySpan;

                       ReturnErrorOnFailure(Credentials::ExtractPublicKeyFromChipCert(trustedCA, trustedCAPublicKeySpan));
                       Crypto::P256PublicKey trustedCAPublicKey{ trustedCAPublicKeySpan };

                       Credentials::P256PublicKeySpan rootPubKeySpan(rootKeySpan.data());
                       Crypto::P256PublicKey fabricTableRootPublicKey{ rootPubKeySpan };

                       if (trustedCAPublicKey.Matches(fabricTableRootPublicKey))
                       {
                            mInfo.adminRCAC = trustedCA;
                            ChipLogProgress(Controller, "JCM: Successfully parsed the Administrator RCAC");
                            foundMatchingRcac = true;
                            break;
                       }
                   }
                   if (!foundMatchingRcac)
                   {
                       ChipLogError(Controller, "JCM: Cannot found a matching RCAC!");
                       return CHIP_ERROR_CERT_NOT_FOUND;
                   }
                   return CHIP_NO_ERROR;
               }
               default:
                   return CHIP_NO_ERROR;
           }
           return CHIP_NO_ERROR;
    });

    if (err != CHIP_NO_ERROR)
    {
        mInfo.adminFabricIndex = kUndefinedFabricIndex;
    }

    return err;
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

CHIP_ERROR JCMCommissioner::FinishReadingCommissioningInfo(ReadCommissioningInfo & info)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    AccumulateErrors(err, ParseAdministratorInfo(info));
    return DeviceCommissioner::FinishReadingCommissioningInfo(info);
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
            mNextStage = JCMTrustVerificationStage::kIdle;
            OnJCMTrustVerificationComplete(&mInfo, result);
            break;
        default:
            ChipLogError(Controller, "Invalid stage: %d", static_cast<int>(mNextStage));
            OnJCMTrustVerificationComplete(nullptr, JCMTrustVerificationResult::kInternalError);
            break;
    }
}

} // namespace Controller
} // namespace chip
