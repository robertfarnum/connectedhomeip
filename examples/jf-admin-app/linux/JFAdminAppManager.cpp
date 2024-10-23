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

#include "JFAdminAppManager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/general-diagnostics-server/general-diagnostics-server.h>
#include <app/clusters/software-diagnostics-server/software-diagnostics-server.h>
#include <app/clusters/switch-server/switch-server.h>
#include <app/server/Server.h>
#include <app/util/att-storage.h>
#include <app/util/attribute-storage.h>
#include <controller/ExampleOperationalCredentialsIssuer.h>
#include <platform/PlatformManager.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/server/Server.h>
#include <app/server/JointFabricDatastorage.h>
#include <lib/support/logging/CHIPLogging.h>
#include <lib/support/TestGroupData.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::Controller;
using namespace chip::Credentials;
using namespace chip::DeviceLayer;
using namespace chip::TLV;

using KeySet = GroupDataProvider::KeySet;

namespace {
   /* demo: fixed for the moment */
   NodeId anchorAdminNodeId = 3;

   static constexpr uint16_t failSafeTimerPeriod = 15; // seconds
}

CHIP_ERROR JFAdminAppManager::Init(Server & server, OperationalCredentialsDelegate & opCredentialsDelegate)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

    mServer             = &server;
    mCASESessionManager = server.GetCASESessionManager();
    mOpCredentials = &opCredentialsDelegate;
    mJointFabricDatastorage = &(mServer->GetJointFabricDatastorage());

    return err;
}

void JFAdminAppManager::HandleCommissioningCompleteEvent()
{
    /* demo: device is initially in its own fabric then onboarded in JF */
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
                /* JFA has Administrator CAT and Anchor/Datastore CAT
                 * when first commissioned in its fabric
                 */
                if (cats.Contains(adminCAT) && cats.Contains(anchorDatastoreCAT))
                {
                    initialFabricIndex = fabricIndex;
                }
                /* after JCM, JFA receives an Administrator CAT */
                else if (cats.Contains(adminCAT) && !cats.Contains(anchorDatastoreCAT))
                {
                    jfFabricIndex = fabricIndex;
                    jfFabricVendorId = fb.GetVendorId();
                }
            }
        }
    }

    if ((initialFabricIndex != kUndefinedFabricIndex) && (jfFabricIndex != kUndefinedFabricIndex))
    {
        ChipLogProgress(JointFabric, "HandleCommissioningCompleteEvent: TriggerJFOnboardingForNextNode state machine");

        anchorAdminScopedNodeId = ScopedNodeId(anchorAdminNodeId, jfFabricIndex);
        TriggerJFOnboardingForNextNode();
    }
    else
    {
        ChipLogProgress(JointFabric, "HandleCommissioningCompleteEvent: Couldn't identify initialFabricIndex and jfFabricIndex.");
    }
}

void JFAdminAppManager::TriggerJFOnboardingForNextNode()
{
    NodeId pendingNodeId = kUndefinedNodeId;
    bool deletedPendingNodesExist = false;

    auto mNodeInformationEntries  = mJointFabricDatastorage->GetNodeInformationEntries();

    for (auto & nodeInformationEntry : mNodeInformationEntries)
    {
        if (nodeInformationEntry.commissioningStatusEntry.state == JointFabricDatastore::DatastoreStateEnum::kCommitted)
        {
            mJointFabricDatastorage->SetNode(nodeInformationEntry.nodeID, JointFabricDatastore::DatastoreStateEnum::kDeletePending);
            pendingNodeId = nodeInformationEntry.nodeID;
            break;
        }
        else if (!deletedPendingNodesExist &&
                 nodeInformationEntry.commissioningStatusEntry.state == JointFabricDatastore::DatastoreStateEnum::kDeletePending)
        {
            deletedPendingNodesExist = true;
        }
    }

    if (pendingNodeId != kUndefinedNodeId)
    {
        pendingScopedNodeId = ScopedNodeId(pendingNodeId, initialFabricIndex);
        ConnectToNode(anchorAdminScopedNodeId, kSendAddPendingNode);
    }
    else if (!deletedPendingNodesExist)
    {
        ChipLogProgress(JointFabric, "All Nodes have now updated NOCs chaining up to the cross-signed ICAC!");
    }
    else
    {
        ChipLogError(JointFabric, "Error while updating NOCs for some of the devices.");

        for (auto & nodeInformationEntry : mNodeInformationEntries)
        {
            if (nodeInformationEntry.commissioningStatusEntry.state == JointFabricDatastore::DatastoreStateEnum::kDeletePending)
            {
                NodeId kDeletePendingNode = nodeInformationEntry.nodeID;

                ChipLogError(JointFabric, "Node ID of device with kDeletePending status: 0x" ChipLogFormatX64, ChipLogValueX64(kDeletePendingNode));
            }
        }
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

    ChipLogDetail(JointFabric, "Establishing session to node ID 0x" ChipLogFormatX64 " on fabric index %d",
                  ChipLogValueX64(scopedNodeId.GetNodeId()), scopedNodeId.GetFabricIndex());

    mCASESessionManager->FindOrEstablishSession(scopedNodeId, &mOnConnectedCallback, &mOnConnectionFailureCallback);
}

void JFAdminAppManager::OnConnected(void * context, Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mSessionHolder.Grab(sessionHandle);

    ChipLogProgress(JointFabric, "OnConnected: successful");

    jfAdminCore->mExchangeMgr = &exchangeMgr;

    switch (jfAdminCore->mOnConnectedAction)
    {
    case kSendAddPendingNode: {
        jfAdminCore->SendAddPendingNode();
        break;
    }
    case kReissueOperationalIdentity: {
        jfAdminCore->SendArmFailSafeTimer();
        break;
    }
    case kSendCommissioningComplete: {
        jfAdminCore->SendCommissioningComplete();
        break;
    }
    case kSendRefreshNode: {
        jfAdminCore->SendRefreshNode();
        break;
    }

    default:
        break;
    }
}

CHIP_ERROR JFAdminAppManager::SendAddPendingNode()
{
	JointFabricDatastore::Commands::AddPendingNode::Type request;

	request.nodeID = pendingScopedNodeId.GetNodeId();
	request.friendlyName = CharSpan::fromCharString("testFriendlyName");

	if (!mExchangeMgr)
	{
        return CHIP_ERROR_UNINITIALIZED;
	}

	ChipLogProgress(JointFabric, "SendAddPendingNode: invoke cluster command.");
	Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
	return cluster.InvokeCommand(request, this, OnAddPendingNodeResponse, OnAddPendingNodeFailure);
}

CHIP_ERROR JFAdminAppManager::SendRefreshNode()
{
	JointFabricDatastore::Commands::RefreshNode::Type request;

	request.nodeID = pendingScopedNodeId.GetNodeId();

	if (!mExchangeMgr)
	{
        return CHIP_ERROR_UNINITIALIZED;
	}

	ChipLogProgress(JointFabric, "SendRefreshNode: invoke cluster command.");
	Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
	return cluster.InvokeCommand(request, this, OnRefreshNodeResponse, OnRefreshFailure);
}

CHIP_ERROR JFAdminAppManager::SendArmFailSafeTimer()
{
    uint64_t breadcrumb = static_cast<uint64_t>(kReissueOperationalIdentity);
    GeneralCommissioning::Commands::ArmFailSafe::Type request;
    request.expiryLengthSeconds = failSafeTimerPeriod;
    request.breadcrumb          = breadcrumb;

    if (!mExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    ChipLogProgress(JointFabric, "SendArmFailSafeTimer: invoke cluster command.");
    Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnArmFailSafeTimerResponse, OnArmFailSafeTimerFailure);
}

CHIP_ERROR JFAdminAppManager::SendCSRRequest()
{
    OperationalCredentials::Commands::CSRRequest::Type request;
    uint8_t csrNonce[kCSRNonceLength];

    if (!mExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    Crypto::DRBG_get_bytes(csrNonce, sizeof(csrNonce));
    request.CSRNonce = csrNonce;

    ChipLogProgress(JointFabric, "SendCSRRequest: invoke cluster command.");
    Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnOperationalCertificateSigningRequest, OnCSRFailureResponse);
}

CHIP_ERROR JFAdminAppManager::SendAddTrustedRootCertificate()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    OperationalCredentials::Commands::AddTrustedRootCertificate::Type request;
    uint8_t pendingRCAC[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan pendingRCACSpan{ pendingRCAC };

    if (!mExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    err = mServer->GetFabricTable().FetchRootCert(this->jfFabricIndex, pendingRCACSpan);
    if (err != CHIP_NO_ERROR || !pendingRCACSpan.size())
    {
        ChipLogProgress(JointFabric, "SendAddTrustedRootCertificate: Error while fetching the JF RCAC!");
        return err;
    }
    request.rootCACertificate = pendingRCACSpan;

    ChipLogProgress(JointFabric, "SendAddTrustedRootCertificate: invoke cluster command.");
    Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnRootCertSuccessResponse, OnRootCertFailureResponse);
}

CHIP_ERROR JFAdminAppManager::SendAddNOC()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    OperationalCredentials::Commands::AddNOC::Type request;
    uint8_t pendingICAC[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan pendingICACSpan{ pendingICAC };

    if (!mExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    /* get ICAC */
    err = mServer->GetFabricTable().FetchICACert(this->jfFabricIndex, pendingICACSpan);
    if (err != CHIP_NO_ERROR || !pendingICACSpan.size())
    {
        ChipLogProgress(JointFabric, "SendAddNOC: Error while fetching the JF ICAC.");
        return err;
    }

    request.NOCValue         = Span(this->pendingNOCSpan.data(), this->pendingNOCSpan.size());
    request.ICACValue        = MakeOptional(Span(pendingICACSpan.data(), pendingICACSpan.size()));
    request.IPKValue         = chip::GroupTesting::DefaultIpkValue::GetDefaultIpk();
    request.caseAdminSubject = NodeIdFromCASEAuthTag(0xFFFF'0001);
    request.adminVendorId    = this->jfFabricVendorId;

    ChipLogProgress(JointFabric, "SendAddNOC: invoke cluster command.");
    Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnOperationalCertificateAddResponse, OnAddNOCFailureResponse);

    return CHIP_NO_ERROR;
}

CHIP_ERROR JFAdminAppManager::SendCommissioningComplete()
{
    GeneralCommissioning::Commands::CommissioningComplete::Type request;

    if (!mExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    ChipLogProgress(JointFabric, "SendCommissioningComplete: invoke cluster command.");
    Controller::ClusterBase cluster(*mExchangeMgr, mSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnCommissioningCompleteResponse, OnCommissioningCompleteFailure);
}

void JFAdminAppManager::DisconnectFromNode()
{
    auto optionalSessionHandle = mSessionHolder.Get();
	if (optionalSessionHandle.HasValue())
	{
	    if (optionalSessionHandle.Value()->IsActiveSession())
	    {
	        optionalSessionHandle.Value()->AsSecureSession()->MarkAsDefunct();
	    }
	}
	mSessionHolder.Release();
}

void JFAdminAppManager::OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "Failed to establish connection to 0x" ChipLogFormatX64 " on fabric index %d",
                  ChipLogValueX64(peerId.GetNodeId()), peerId.GetFabricIndex());

    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnAddPendingNodeResponse(void * context, const chip::app::DataModel::NullObjectType &)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(JointFabric, "OnAddPendingNodeResponse!");

    /* disconnect from the Anchor Administrator */
    jfAdminCore->DisconnectFromNode();

    /* start device onboarding into JF */
    jfAdminCore->ConnectToNode(jfAdminCore->pendingScopedNodeId, kReissueOperationalIdentity);
}

void JFAdminAppManager::OnAddPendingNodeFailure(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->DisconnectFromNode();

    ChipLogError(JointFabric, "OnAddPendingNodeFailure!");
}

void JFAdminAppManager::OnArmFailSafeTimerResponse(void * context, const app::Clusters::GeneralCommissioning::Commands::ArmFailSafeResponse::DecodableType & data)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(JointFabric, "OnArmFailSafeTimerResponse, errorCode=%u", to_underlying(data.errorCode));

    jfAdminCore->SendCSRRequest();
}

void JFAdminAppManager::OnArmFailSafeTimerFailure(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "OnArmFailSafeTimerFailure!");
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnOperationalCertificateSigningRequest(void * context, const OperationalCredentials::Commands::CSRResponse::DecodableType & data)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    uint8_t icac[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan ICACSpan{ icac };

    uint8_t ownJFNoc[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan ownJFNocSpan{ ownJFNoc };
    uint8_t ownJFNocDer[Credentials::kMaxDERCertLength] = {0};
    MutableByteSpan ownJFNocDerSpan{ ownJFNocDer };
    ChipDN ownJFNocDn;
    uint64_t dnFabricIDJFNoc;

    uint8_t icacDer[Credentials::kMaxDERCertLength] = { 0 };
    MutableByteSpan ICACDerSpan{ icacDer };

    uint8_t noc[Credentials::kMaxCHIPCertLength] = { 0 };
    MutableByteSpan nocSpan{ noc };

    uint8_t nocDer[Credentials::kMaxDERCertLength] = { 0 };
    MutableByteSpan nocDerSpan{ nocDer };

    TLVReader reader;
    TLVType containerType;

    ByteSpan csrSpan;

    ChipLogProgress(JointFabric, "OnOperationalCertificateSigningRequest!");

    /* extract CSR from the CSRResponse */
    reader.Init(data.NOCSRElements);
    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }
    err = reader.Expect(kTLVType_Structure, AnonymousTag());
    SuccessOrExit(err);

    err = reader.EnterContainer(containerType);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_ByteString, TLV::ContextTag(1));
    SuccessOrExit(err);

    csrSpan = Span(reader.GetReadPoint(), reader.GetLength());
    reader.ExitContainer(containerType);

    /* extract ICAC */
    err = jfAdminCore->mServer->GetFabricTable().FetchICACert(jfAdminCore->jfFabricIndex, ICACSpan);
    SuccessOrExit(err);

    /* extract JF NOC: needed for Matter Operational Certificate DN attribute for fabric identifier */
    err = jfAdminCore->mServer->GetFabricTable().FetchNOCCert(jfAdminCore->jfFabricIndex, ownJFNocSpan);
    SuccessOrExit(err);

    err = ConvertChipCertToX509Cert(ownJFNocSpan, ownJFNocDerSpan);
    SuccessOrExit(err);

    ownJFNocDn = ChipDN{};
    err = ExtractSubjectDNFromX509Cert(ownJFNocDerSpan, ownJFNocDn);
    SuccessOrExit(err);

    err = ownJFNocDn.GetCertFabricId(dnFabricIDJFNoc);
    SuccessOrExit(err);

    jfAdminCore->mOpCredentials->SetNodeIdForNextNOCRequest(jfAdminCore->pendingScopedNodeId.GetNodeId());
    jfAdminCore->mOpCredentials->SetFabricIdForNextNOCRequest(dnFabricIDJFNoc);

    err = ConvertChipCertToX509Cert(ICACSpan, ICACDerSpan);
    SuccessOrExit(err);

    err = jfAdminCore->mOpCredentials->SignNOC(ICACDerSpan, csrSpan, nocDerSpan);
    SuccessOrExit(err);

    err = ConvertX509CertToChipCert(nocDerSpan, nocSpan);
    SuccessOrExit(err);

    CopySpanToMutableSpan(nocSpan, jfAdminCore->pendingNOCSpan);

exit:
    if (CHIP_NO_ERROR == err)
    {
        jfAdminCore->SendAddTrustedRootCertificate();
    }
    else
    {
        ChipLogError(JointFabric, "Error during OnOperationalCertificateSigningRequest");
        jfAdminCore->DisconnectFromNode();
        jfAdminCore->TriggerJFOnboardingForNextNode();
    }
}

void JFAdminAppManager::OnCSRFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(JointFabric, "OnCSRFailureResponse!");
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnRootCertSuccessResponse(void * context, const chip::app::DataModel::NullObjectType &)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(JointFabric, "OnRootCertSuccessResponse!");

    jfAdminCore->SendAddNOC();
}

void JFAdminAppManager::OnRootCertFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "OnRootCertFailureResponse!");
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnOperationalCertificateAddResponse(
    void * context, const OperationalCredentials::Commands::NOCResponse::DecodableType & data)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    err = ConvertFromOperationalCertStatus(data.statusCode);

    ChipLogProgress(JointFabric, "OnOperationalCertificateAddResponse: %s!", ErrorStr(err));

    jfAdminCore->DisconnectFromNode();

    if (CHIP_NO_ERROR == err)
    {
        /* switch to JF fabric from now: Commissioning Complete requests/response commands */
        jfAdminCore->pendingScopedNodeId = ScopedNodeId(jfAdminCore->pendingScopedNodeId.GetNodeId(), jfAdminCore->jfFabricIndex);
        jfAdminCore->ConnectToNode(jfAdminCore->pendingScopedNodeId, kSendCommissioningComplete);
    }
    else
    {
        jfAdminCore->TriggerJFOnboardingForNextNode();
    }
}

void JFAdminAppManager::OnAddNOCFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "OnAddNOCFailureResponse!");
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnCommissioningCompleteResponse(
    void * context, const GeneralCommissioning::Commands::CommissioningCompleteResponse::DecodableType & data)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->DisconnectFromNode();

    ChipLogProgress(JointFabric, "OnCommissioningCompleteResponse, Code=%u", to_underlying(data.errorCode));

    /* CommissioningCompleteResponse has errors, proceed with JF onboarding another node */
    if (data.errorCode != GeneralCommissioning::CommissioningErrorEnum::kOk)
    {
        jfAdminCore->TriggerJFOnboardingForNextNode();
    }
    else
    {
        jfAdminCore->ConnectToNode(jfAdminCore->anchorAdminScopedNodeId, kSendRefreshNode);
    }
}

void JFAdminAppManager::OnCommissioningCompleteFailure(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "Received failure response %s\n", chip::ErrorStr(error));
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnRefreshNodeResponse(void * context, const chip::app::DataModel::NullObjectType &)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(JointFabric, "OnRefreshNodeResponse!");

    /* node is marked as kCommitted on JFA, remove it from the local Datastore */
    jfAdminCore->mJointFabricDatastorage->RemoveNode(jfAdminCore->pendingScopedNodeId.GetNodeId());

    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

void JFAdminAppManager::OnRefreshFailure(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogError(JointFabric, "OnRefreshNodeFailure!");
    jfAdminCore->DisconnectFromNode();
    jfAdminCore->TriggerJFOnboardingForNextNode();
}

CHIP_ERROR JFAdminAppManager::ConvertFromOperationalCertStatus(OperationalCredentials::NodeOperationalCertStatusEnum err)
{
    using OperationalCredentials::NodeOperationalCertStatusEnum;
    switch (err)
    {
    case NodeOperationalCertStatusEnum::kOk:
        return CHIP_NO_ERROR;
    case NodeOperationalCertStatusEnum::kInvalidPublicKey:
        return CHIP_ERROR_INVALID_PUBLIC_KEY;
    case NodeOperationalCertStatusEnum::kInvalidNodeOpId:
        return CHIP_ERROR_WRONG_NODE_ID;
    case NodeOperationalCertStatusEnum::kInvalidNOC:
        return CHIP_ERROR_UNSUPPORTED_CERT_FORMAT;
    case NodeOperationalCertStatusEnum::kMissingCsr:
        return CHIP_ERROR_INCORRECT_STATE;
    case NodeOperationalCertStatusEnum::kTableFull:
        return CHIP_ERROR_NO_MEMORY;
    case NodeOperationalCertStatusEnum::kInvalidAdminSubject:
        return CHIP_ERROR_INVALID_ADMIN_SUBJECT;
    case NodeOperationalCertStatusEnum::kFabricConflict:
        return CHIP_ERROR_FABRIC_EXISTS;
    case NodeOperationalCertStatusEnum::kLabelConflict:
        return CHIP_ERROR_INVALID_ARGUMENT;
    case NodeOperationalCertStatusEnum::kInvalidFabricIndex:
        return CHIP_ERROR_INVALID_FABRIC_INDEX;
    case NodeOperationalCertStatusEnum::kUnknownEnumValue:
        // Is this a reasonable value?
        return CHIP_ERROR_CERT_LOAD_FAILED;
    }

    return CHIP_ERROR_CERT_LOAD_FAILED;
}
