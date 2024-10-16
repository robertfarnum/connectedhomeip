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
#include <controller/ExampleOperationalCredentialsIssuer.h>
#include <platform/PlatformManager.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/server/Server.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::Credentials;
using namespace chip::DeviceLayer;
using namespace chip::TLV;
using namespace chip::Controller;

using KeySet = GroupDataProvider::KeySet;

CHIP_ERROR JFAdminAppManager::Init(Server & server, OperationalCredentialsDelegate & opCredentialsDelegate)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

    mServer             = &server;
    mCASESessionManager = server.GetCASESessionManager();
    mOpCredentials = &opCredentialsDelegate;

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
                    this->jfFabricVendorId = fb.GetVendorId();
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

    err = mServer->GetFabricTable().FetchRootCert(this->jfFabricIndex, pendingRCACSpan);
    if (err != CHIP_NO_ERROR || !pendingRCACSpan.size())
    {
        ChipLogProgress(DeviceLayer, "Error while fetching JF RCAC!");
        return err;
    }
    request.rootCACertificate = pendingRCACSpan;

    Controller::ClusterBase cluster(*mPendingExchangeMgr, mPendingSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnRootCertSuccessResponse, OnRootCertFailureResponse);
}

CHIP_ERROR JFAdminAppManager::SendCSRRequest()
{
    OperationalCredentials::Commands::CSRRequest::Type request;
    uint8_t csrNonce[kCSRNonceLength];

    if (!mPendingExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    Crypto::DRBG_get_bytes(csrNonce, sizeof(csrNonce));
    request.CSRNonce = csrNonce;

    Controller::ClusterBase cluster(*mPendingExchangeMgr, mPendingSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnOperationalCertificateSigningRequest, OnCSRFailureResponse);
}

CHIP_ERROR JFAdminAppManager::SendAddNOC(MutableByteSpan nocSpan)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    OperationalCredentials::Commands::AddNOC::Type request;
    uint8_t pendingICAC[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan pendingICACSpan{ pendingICAC };

    auto * groupDataProvider = Credentials::GetGroupDataProvider();
    KeySet keyset;

    if (!mPendingExchangeMgr)
    {
        return CHIP_ERROR_UNINITIALIZED;
    }

    /* get ICAC */
    err = mServer->GetFabricTable().FetchICACert(this->jfFabricIndex, pendingICACSpan);
    if (err != CHIP_NO_ERROR || !pendingICACSpan.size())
    {
        ChipLogProgress(DeviceLayer, "Error while fetching JF ICAC!");
        return err;
    }

    /* get IPK value */
    err = groupDataProvider->GetKeySet(this->jfFabricIndex, Credentials::GroupDataProvider::kIdentityProtectionKeySetId, keyset);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogProgress(DeviceLayer, "Error while fetching the IPK.");
        return err;
    }

    request.NOCValue         = Span(nocSpan.data(), nocSpan.size());
    request.ICACValue        = MakeOptional(Span(pendingICACSpan.data(), pendingICACSpan.size()));
    request.IPKValue         = keyset.epoch_keys[0].key;
    request.caseAdminSubject = NodeIdFromCASEAuthTag(0xFFFF'0001);
    request.adminVendorId    = this->jfFabricVendorId;

    Controller::ClusterBase cluster(*mPendingExchangeMgr, mPendingSessionHolder.Get().Value(), kRootEndpointId);
    return cluster.InvokeCommand(request, this, OnOperationalCertificateAddResponse, OnAddNOCFailureResponse);

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
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(DeviceLayer, "OnRootCertSuccessResponse!");

    jfAdminCore->SendCSRRequest();
}

void JFAdminAppManager::OnRootCertFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Release();

    ChipLogProgress(DeviceLayer, "OnRootCertFailureResponse!");
}

void JFAdminAppManager::OnOperationalCertificateSigningRequest(void * context, const OperationalCredentials::Commands::CSRResponse::DecodableType & data)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    uint8_t icac[Credentials::kMaxCHIPCertLength] = {0};
    MutableByteSpan ICACSpan{ icac };

    uint8_t icacDer[Credentials::kMaxDERCertLength] = { 0 };
    MutableByteSpan ICACDerSpan{ icacDer };

    uint8_t noc[Credentials::kMaxCHIPCertLength] = { 0 };
    MutableByteSpan nocSpan{ noc };

    uint8_t nocDer[Credentials::kMaxDERCertLength] = { 0 };
    MutableByteSpan nocDerSpan{ nocDer };

    TLVReader reader;
    TLVType containerType;

    ChipLogProgress(DeviceLayer, "OnOperationalCertificateSigningRequest!");

    /* extract CSR from the CSRResponse */
    reader.Init(data.NOCSRElements);
    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        if (err != CHIP_NO_ERROR)
        {
           ChipLogProgress(DeviceLayer, "Error while processing NOCSRElements! (reader.GetType())");
           return;
        }
    }
    if (reader.Expect(kTLVType_Structure, AnonymousTag()) != CHIP_NO_ERROR)
    {
        ChipLogProgress(DeviceLayer, "Error while processing NOCSRElements! (AnonymousTag) ");
        return;
    }
    if (reader.EnterContainer(containerType) != CHIP_NO_ERROR)
    {
        ChipLogProgress(DeviceLayer, "Error while processing NOCSRElements! (EnterContainer) ");
        return;
    }
    if (reader.Next(kTLVType_ByteString, TLV::ContextTag(1)) != CHIP_NO_ERROR)
    {
        ChipLogProgress(DeviceLayer, "Error while processing NOCSRElements! (kTLVType_ByteString) ");
        return;
    }

    ByteSpan csrSpan(reader.GetReadPoint(), reader.GetLength());
    reader.ExitContainer(containerType);

    /* extract ICAC */
    err = jfAdminCore->mServer->GetFabricTable().FetchICACert(jfAdminCore->jfFabricIndex, ICACSpan);
    if (err != CHIP_NO_ERROR || !ICACSpan.size())
    {
       ChipLogProgress(DeviceLayer, "Error while fetching JF ICAC!");
       return;
    }

    err = ConvertChipCertToX509Cert(ICACSpan, ICACDerSpan);
    if (err != CHIP_NO_ERROR || !ICACDerSpan.size())
    {
       ChipLogProgress(DeviceLayer, "Error during conversion to DER!");
       return;
    }

    jfAdminCore->mOpCredentials->SetNodeIdForNextNOCRequest(jfAdminCore->pendingNodeId.GetNodeId());
    jfAdminCore->mOpCredentials->SetFabricIdForNextNOCRequest(jfAdminCore->jfFabricIndex);

    err = jfAdminCore->mOpCredentials->SignNOC(ICACDerSpan, csrSpan, nocDerSpan);
    if (err != CHIP_NO_ERROR || !nocDerSpan.size())
    {
        ChipLogProgress(DeviceLayer, "Error during SignNOC!");
        return;
    }

    err = ConvertX509CertToChipCert(nocDerSpan, nocSpan);
    if (err != CHIP_NO_ERROR || !nocSpan.size())
    {
       ChipLogProgress(DeviceLayer, "Error during conversion to DER!");
       return;
    }

    jfAdminCore->SendAddNOC(nocSpan);
}

void JFAdminAppManager::OnCSRFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Release();

    ChipLogProgress(DeviceLayer, "OnCSRFailureResponse!");
}

void JFAdminAppManager::OnAddNOCFailureResponse(void * context, CHIP_ERROR error)
{
    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);
    jfAdminCore->mPendingSessionHolder.Release();

    ChipLogProgress(DeviceLayer, "OnAddNOCFailureResponse!");
}

void JFAdminAppManager::OnOperationalCertificateAddResponse(
    void * context, const OperationalCredentials::Commands::NOCResponse::DecodableType & data)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    JFAdminAppManager * jfAdminCore = static_cast<JFAdminAppManager *>(context);
    VerifyOrDie(jfAdminCore != nullptr);

    ChipLogProgress(DeviceLayer, "OnOperationalCertificateAddResponse: %s!", ErrorStr(err));

    auto optionalSessionHandle = jfAdminCore->mPendingSessionHolder.Get();
    if (optionalSessionHandle.HasValue())
    {
        if (optionalSessionHandle.Value()->IsActiveSession())
        {
            optionalSessionHandle.Value()->AsSecureSession()->MarkAsDefunct();
        }
    }
    jfAdminCore->mPendingSessionHolder.Release();
}

// Called whenever FindOrEstablishSession fails
void JFAdminAppManager::OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error)
{
    ChipLogProgress(DeviceLayer, "Failed to connected to Node!");
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
