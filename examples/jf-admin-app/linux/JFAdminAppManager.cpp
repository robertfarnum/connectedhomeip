/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "JFAdminAppManager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/general-diagnostics-server/general-diagnostics-server.h>
#include <app/clusters/software-diagnostics-server/software-diagnostics-server.h>
#include <app/clusters/switch-server/switch-server.h>
#include <app/server/Server.h>
#include <app/util/att-storage.h>
#include <app/util/attribute-storage.h>
#include <platform/PlatformManager.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/server/Server.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::DeviceLayer;

CHIP_ERROR JFAdminAppManager::Init(Server & server)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

    mServer             = &server;
    mCASESessionManager = server.GetCASESessionManager();

    return err;
}

void JFAdminAppManager::HandleCommissioningCompleteEvent()
{
    /* demo: identity the index of the JF fabric */
    if (Server::GetInstance().GetFabricTable().FabricCount() == 2)
    {
        for (const auto & fb : mServer->GetFabricTable())
        {
            FabricIndex fabricIndex = fb.GetFabricIndex();
            CASEAuthTag adminCAT = 0xFFFF'0001;
            CASEAuthTag anchorDatastoreCAT = 0xFFFC'0001;
            CATValues cats;

            if (mServer->GetFabricTable().FetchCATs(fabricIndex, cats) == CHIP_NO_ERROR)
            {
                if (cats.Contains(adminCAT) && cats.Contains(anchorDatastoreCAT))
                {
                    this->initialFabricIndex = fabricIndex;
                }
                else if (cats.Contains(adminCAT) && !cats.Contains(anchorDatastoreCAT))
                {
                    this->jfFabricIndex = fabricIndex;
                }
            }
        }
    }

    if ((this->initialFabricIndex != kUndefinedFabricIndex) && (this->jfFabricIndex != kUndefinedFabricIndex))
    {
        ChipLogProgress(DeviceLayer, "Will trigger addNOC/addRCAC using the cross-signed ICAC.")

        /* fixed node for the moment, have to iterate through the Datastore */
        NodeId fixedNodeId = 10;
        ScopedNodeId scopedNodeId = ScopedNodeId(fixedNodeId, this->initialFabricIndex);

        this->pendingNodeId = ScopedNodeId(scopedNodeId.GetNodeId(), scopedNodeId.GetFabricIndex());
        this->ConnectToNode(scopedNodeId, kArmFailSafeTimer);
    }
    else
    {
        ChipLogError(DeviceLayer, "Couldn't find initialFabricIndex and jfFabricIndex.");
    }
}

void JFAdminAppManager::ConnectToNode(ScopedNodeId scopedNodeId, OnConnectedAction onConnectedAction)
{
    VerifyOrDie(mServer != nullptr);

    if ((scopedNodeId.GetFabricIndex() == kUndefinedFabricIndex) ||
        (scopedNodeId.GetNodeId() == kUndefinedNodeId))
    {
        ChipLogError(DeviceLayer, "Invalid node location!");
        return;
    }

    // Set the action to take once connection is successfully established
    mOnConnectedAction = onConnectedAction;

    ChipLogDetail(DeviceLayer, "Establishing session to provider node ID 0x" ChipLogFormatX64 " on fabric index %d",
                  ChipLogValueX64(scopedNodeId.GetNodeId()), scopedNodeId.GetFabricIndex());

    mCASESessionManager->FindOrEstablishSession(scopedNodeId, &mOnConnectedCallback, &mOnConnectionFailureCallback);
}

// Called whenever FindOrEstablishSession is successful
void JFAdminAppManager::OnConnected(void * context, Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Grab(sessionHandle);

    ChipLogProgress(DeviceLayer, "Connected to Node!");

    jfAdminCore->mPendingExchangeMgr = &exchangeMgr;

    switch (jfAdminCore->mOnConnectedAction)
    {
    case kArmFailSafeTimer: {
        jfAdminCore->SendArmFailSafeTimer();
        break;
    }

    default:
        break;
    }
}

CHIP_ERROR JFAdminAppManager::SendArmFailSafeTimer()
{
    uint64_t breadcrumb = static_cast<uint64_t>(kArmFailSafeTimer);
	GeneralCommissioning::Commands::ArmFailSafe::Type request;
	request.expiryLengthSeconds = 15;
	request.breadcrumb          = breadcrumb;

    if (!mPendingExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    Controller::ClusterBase cluster(*mPendingExchangeMgr, mPendingSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnArmFailSafeTimerResponse, OnArmFailSafeTimerFailure);
}

CHIP_ERROR JFAdminAppManager::SendAddTrustedRootCertificate()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    OperationalCredentials::Commands::AddTrustedRootCertificate::Type request;
    uint8_t pendingRCAC[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan pendingRCACSpan{ pendingRCAC };

    if (!mPendingExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    err = mServer->GetFabricTable().FetchRootCert(this->initialFabricIndex, pendingRCACSpan);
    if (err != CHIP_NO_ERROR || !pendingRCACSpan.size())
    {
        ChipLogProgress(DeviceLayer, "Error while fetching JF RCAC!");
        return err;
    }
    request.rootCACertificate = pendingRCACSpan;

    Controller::ClusterBase cluster(*mPendingExchangeMgr, mPendingSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnRootCertSuccessResponse, OnRootCertFailureResponse);
}

CHIP_ERROR JFAdminAppManager::SendAddNOC()
{
	return CHIP_NO_ERROR;
}

CHIP_ERROR JFAdminAppManager::SendDisarmFailSafeTimer()
{
	return CHIP_NO_ERROR;
}

void JFAdminAppManager::OnArmFailSafeTimerResponse(void * context, const app::Clusters::GeneralCommissioning::Commands::ArmFailSafeResponse::DecodableType & data)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(DeviceLayer, "Received ArmFailSafe response errorCode=%u", to_underlying(data.errorCode));

    jfAdminCore->SendAddTrustedRootCertificate();
}

void JFAdminAppManager::OnArmFailSafeTimerFailure(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Release();

    ChipLogProgress(DeviceLayer, "OnArmFailSafeTimerFailure!");
}

void JFAdminAppManager::OnRootCertSuccessResponse(void * context, const chip::app::DataModel::NullObjectType &)
{
    ChipLogProgress(DeviceLayer, "OnRootCertSuccessResponse!");
}

void JFAdminAppManager::OnRootCertFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Release();

    ChipLogProgress(DeviceLayer, "OnRootCertFailureResponse!");
}

// Called whenever FindOrEstablishSession fails
void JFAdminAppManager::OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error)
{
    ChipLogProgress(DeviceLayer, "Failed to connected to Node!");
}
