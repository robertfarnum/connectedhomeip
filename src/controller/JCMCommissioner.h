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
#include <controller/CHIPDeviceController.h>
#include <controller/JCMTrustVerification.h>
#include <app/DeviceProxy.h>
#include <app/ReadClient.h>
#include <lib/core/CHIPCallback.h>
#include <lib/core/CHIPError.h>
#if CONFIG_DEVICE_LAYER
#include <platform/CHIPDeviceLayer.h>
#endif

namespace chip {

namespace Controller {

class JCMCommissioner : public DeviceCommissioner
{
public:
    JCMCommissioner(){};
    ~JCMCommissioner(){};
    CHIP_ERROR StartJCMTrustVerification(DeviceProxy * proxy) override;
    void OnJCMTrustVerificationComplete(const JCMTrustVerificationInfo  *info, JCMTrustVerificationResult result);
    void RegisterJCMTrustVerificationDelegate(JCMTrustVerificationDelegate * jcmTrustVerificationDelegate)
    {
        mJCMTrustVerificationDelegate = jcmTrustVerificationDelegate;
    }
    void ContinueAfterUserConsent(bool consent);

    void OnDone(app::ReadClient *) override;

protected:
    // Override FinishReadingCommissioningInfo to parse JCM administrator info
    CHIP_ERROR FinishReadingCommissioningInfo(ReadCommissioningInfo & info) override;

private:
    //CHIP_ERROR SendCommissioningReadRequest(Optional<System::Clock::Timeout> timeout, app::AttributePathParams * readPaths, size_t readPathsSize);
    
    void FindAdministratorEndpoint();

    // Move to next JCM commissioning step
    void AdvanceTrustVerificationStage(JCMTrustVerificationResult result);

    // Parse the JCM administrator info from the commissioning info
    CHIP_ERROR ParseAdministratorInfo(ReadCommissioningInfo & info);

    // JCM commissioning steps
    void DiscoverAdministratorEndpoint();
    void ReadAdministratorFabricIndex();
    void PerformVendorIDVerificationProcedure();
    void VerifyNOCContainsAdministratorCAT();
    void AskUserForConsent();

    JCMTrustVerificationStage mNextStage = JCMTrustVerificationStage::kIdle;
    JCMTrustVerificationDelegate * mJCMTrustVerificationDelegate = nullptr;
    DeviceProxy * mDeviceProxy = nullptr;

    JCMTrustVerificationInfo mInfo;
};

} // namespace Controller
} // namespace chip