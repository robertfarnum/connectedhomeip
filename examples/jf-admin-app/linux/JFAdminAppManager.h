/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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

#include <AppMain.h>

namespace chip {

class JFAdminAppManager
{
public:
    JFAdminAppManager() : mOnConnectedCallback(OnConnected, this), mOnConnectionFailureCallback(OnConnectionFailure, this) {}

    void HandleCommissioningCompleteEvent();

    CHIP_ERROR Init(Server & server, chip::Controller::OperationalCredentialsDelegate & opCredentialsDelegate);

private:
    // Various actions to take when OnConnected callback is called
    enum OnConnectedAction
    {
        kArmFailSafeTimer = 0,
        kAddTrustedRoot,
        kAddNOC,
    };

    void ConnectToNode(chip::ScopedNodeId scopedNodeId, OnConnectedAction onConnectedAction);
    CHIP_ERROR SendArmFailSafeTimer();
    CHIP_ERROR SendAddTrustedRootCertificate();
    CHIP_ERROR SendCSRRequest();
    CHIP_ERROR SendAddNOC(MutableByteSpan nocSpan);

    static void OnConnected(void * context, Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    static void OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error);
    Callback::Callback<OnDeviceConnected> mOnConnectedCallback;
    Callback::Callback<OnDeviceConnectionFailure> mOnConnectionFailureCallback;

    static void OnArmFailSafeTimerResponse(void * context, const app::Clusters::GeneralCommissioning::Commands::ArmFailSafeResponse::DecodableType & data);
    static void OnArmFailSafeTimerFailure(void * context, CHIP_ERROR error);

    static void OnRootCertSuccessResponse(void * context, const chip::app::DataModel::NullObjectType &);
    static void OnRootCertFailureResponse(void * context, CHIP_ERROR error);

    static void OnOperationalCertificateSigningRequest(void * context, const app::Clusters::OperationalCredentials::Commands::CSRResponse::DecodableType & data);
    static void OnCSRFailureResponse(void * context, CHIP_ERROR error);

    static void OnAddNOCFailureResponse(void * context, CHIP_ERROR error);
    static void OnOperationalCertificateAddResponse(void * context, const app::Clusters::OperationalCredentials::Commands::NOCResponse::DecodableType & data);

    static CHIP_ERROR ConvertFromOperationalCertStatus(app::Clusters::OperationalCredentials::NodeOperationalCertStatusEnum err);

    OnConnectedAction mOnConnectedAction = kArmFailSafeTimer;
    Server * mServer = nullptr;
    CASESessionManager * mCASESessionManager = nullptr;
    chip::Controller::OperationalCredentialsDelegate * mOpCredentials = nullptr;

    FabricIndex initialFabricIndex = kUndefinedFabricIndex;
    FabricIndex jfFabricIndex = kUndefinedFabricIndex;
    VendorId jfFabricVendorId = NotSpecified;

    ScopedNodeId pendingNodeId;
    Messaging::ExchangeManager * mPendingExchangeMgr = nullptr;
    SessionHolder mPendingSessionHolder;
};

} // namespace chip
