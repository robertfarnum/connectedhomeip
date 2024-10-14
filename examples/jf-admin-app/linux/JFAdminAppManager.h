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

    CHIP_ERROR Init(Server & server);

private:
    // Various actions to take when OnConnected callback is called
    enum OnConnectedAction
    {
        kArmFailSafeTimer = 0,
        kAddTrustedRoot,
        kAddNOC,
        kDisarmFailSafeTimer,
    };

    void ConnectToNode(chip::ScopedNodeId scopedNodeId, OnConnectedAction onConnectedAction);
    CHIP_ERROR SendArmFailSafeTimer(Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    CHIP_ERROR SendAddTrustedRootCertificate(Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    CHIP_ERROR SendAddNOC(Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    CHIP_ERROR SendDisarmFailSafeTimer(Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);

    static void OnConnected(void * context, Messaging::ExchangeManager & exchangeMgr, const SessionHandle & sessionHandle);
    static void OnConnectionFailure(void * context, const ScopedNodeId & peerId, CHIP_ERROR error);
    Callback::Callback<OnDeviceConnected> mOnConnectedCallback;
    Callback::Callback<OnDeviceConnectionFailure> mOnConnectionFailureCallback;

    static void OnArmFailSafeTimerResponse(void * context, const app::Clusters::GeneralCommissioning::Commands::ArmFailSafeResponse::DecodableType & data);
    static void OnArmFailSafeTimerFailure(void * context, CHIP_ERROR error);

    OnConnectedAction mOnConnectedAction = kArmFailSafeTimer;
    Server * mServer = nullptr;
    CASESessionManager * mCASESessionManager = nullptr;
};

} // namespace chip
