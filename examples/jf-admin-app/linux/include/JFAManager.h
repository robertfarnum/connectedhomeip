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

#include <AppMain.h>
#include <app/server/JointFabricAdministrator.h>
#include <app/server/Server.h>

#include <functional>
#include <stdbool.h>
#include <stdint.h>

#include <lib/core/CHIPError.h>

#include "controller/jcm/AutoCommissioner.h"
#include "controller/jcm/DeviceCommissioner.h"
#include "joint_fabric/ExampleOperationalCredentialsIssuer.h"

namespace chip {

class JFAManager : public app::JointFabricAdministrator::Delegate
{
public:
    JFAManager() : mOnConnectedCallback(OnConnected, this), mOnConnectionFailureCallback(OnConnectionFailure, this) {}

    CHIP_ERROR Init(Server & server);
    void HandleCommissioningCompleteEvent();

    /* app::JointFabricAdministrator::Delegate */
    CHIP_ERROR GetIcacCsr(MutableByteSpan & icacCsr) override;

    CHIP_ERROR GetJointFabricMode(uint8_t & jointFabricMode);

    CHIP_ERROR StartJcm(NodeId nodeId, EndpointId peerAdminEndpointId);
    CHIP_ERROR SignIcac(ByteSpan icacCsr, FabricId anchorFabricId, MutableByteSpan & icac);

    bool IsDeviceCommissioned() { return jfFabricIndex != kUndefinedFabricId; }
    bool IsDeviceJFAdmin();
    bool IsDeviceJFAnchor();

private:
    // Various actions to take when OnConnected callback is called
    enum OnConnectedAction
    {
        kStandardCommissioningComplete = 0,
        kJCMCommissioning              = 1,
    };

    friend JFAManager & JFAMgr(void);

    static JFAManager sJFA;
    static void OnConnected(void * context, Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    static void OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error);

    Server * mServer                          = nullptr;
    CASESessionManager * mCASESessionManager  = nullptr;
    Messaging::ExchangeManager * mExchangeMgr = nullptr;
    chip::CustomCredentialsIssuer::ExampleOperationalCredentialsIssuer mOpCredsIssuer;
    chip::Controller::JCM::DeviceCommissioner mDeviceCommissioner;
    chip::Controller::JCM::AutoCommissioner mAutoCommissioner;
    SessionHolder mSessionHolder;
    Callback::Callback<OnDeviceConnected> mOnConnectedCallback;
    Callback::Callback<OnDeviceConnectionFailure> mOnConnectionFailureCallback;
    NodeId mNodeId                               = kUndefinedNodeId;
    OnConnectedAction mOnConnectedAction         = kStandardCommissioningComplete;
    FabricId jfFabricIndex                       = kUndefinedFabricId;
    EndpointId peerAdminJFAdminClusterEndpointId = kInvalidEndpointId;
    Crypto::P256PublicKey peerAdminICACPubKey;
    uint8_t mICACBuffer[Credentials::kMaxDERCertLength];
    size_t mICACBufferLen = 0;

    void ConnectToNode(ScopedNodeId scopedNodeId, OnConnectedAction onConnectedAction);
    CHIP_ERROR SendCommissioningComplete();
    CHIP_ERROR AnnounceJointFabricAdministrator();
    CHIP_ERROR SendICACSRRequest();

    static void OnCommissioningCompleteResponse(
        void * context, const app::Clusters::GeneralCommissioning::Commands::CommissioningCompleteResponse::DecodableType & data);
    static void OnCommissioningCompleteFailure(void * context, CHIP_ERROR error);
    static void OnAnnounceJointFabricAdministratorResponse(void * context, const chip::app::DataModel::NullObjectType &);
    static void OnAnnounceJointFabricAdministratorFailure(void * context, CHIP_ERROR error);
    static void
    OnSendICACSRRequestResponse(void * context,
                                const app::Clusters::JointFabricAdministrator::Commands::ICACCSRResponse::DecodableType & icaccsr);
    static void OnSendICACSRRequestFailure(void * context, CHIP_ERROR error);

    void ReleaseSession();
};

inline JFAManager & JFAMgr(void)
{
    return JFAManager::sJFA;
}

} // namespace chip
