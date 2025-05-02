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

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/core/CHIPVendorIdentifiers.hpp>
#include <lib/support/DLLUtil.h>

namespace chip {
namespace Controller {
 
struct JCMTrustVerificationInfo {
    EndpointId endPointId;
};

enum class JCMTrustVerificationResult : uint16_t
{
    kSuccess = 0,

    kNotAnAdministrator  = 100,
    // TODO: Add more JCM trust verification errors

    kNoMemory = 700,

    kInvalidArgument = 800,

    kInternalError = 900,

    kNotImplemented = 0xFFFFU,
};

struct JCMTrustVerificationError
{
    JCMTrustVerificationError(JCMTrustVerificationResult result) : mResult(result) {}
    JCMTrustVerificationResult mResult;
};

enum JCMTrustVerificationStage : uint8_t
{
    kIdle,
    kStarted,
    kDiscoveringAdministratorEndpoint,
    kReadingAdministratorFabricIndex,
    kPerformingVendorIDVerificationProcedure,
    kVerifyingNOCContainsAdministratorCAT,
};

typedef void (*JCMTrustVerificationCompleteCallback)(void * context, JCMTrustVerificationInfo * info, JCMTrustVerificationResult result);

/**
 * A delegate that can be notified of progress as the JCM Trust Verification check proceeds.
 */
class DLL_EXPORT JCMTrustVerificationDelegate
{
public:
    virtual ~JCMTrustVerificationDelegate() {}

    virtual bool onProgressUpdate(JCMTrustVerificationStage stage, JCMTrustVerificationError error)
    {
        return false;
    }
    virtual bool OnAskUserForConsent(VendorId vendorId) { return false; }
};

} // namespace Controller
} // namespace chip