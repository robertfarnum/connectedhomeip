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

void JFAdminAppManager::HandleCommissioningCompleteEvent()
{
    /* demo: identity the index of the JF fabric */
    if (Server::GetInstance().GetFabricTable().FabricCount() == 2)
    {
        for (const auto & fb : Server::GetInstance().GetFabricTable())
        {
            FabricIndex fabricIndex = fb.GetFabricIndex();
            CASEAuthTag adminCAT = 0xFFFF'0001;
            CATValues cats;

            /* demo: NOC from JF contains an Administrator CAT */
            if (Server::GetInstance().GetFabricTable().FetchCATs(fabricIndex, cats) == CHIP_NO_ERROR)
            {
                if (cats.Contains(adminCAT))
                {
                    ChipLogProgress(DeviceLayer, "JF found! Will trigger addNOC/addRCAC using the cross-signed ICAC.");
                    break;
                }
            }
        }
    }
}

void JFAdminAppManager::ConnectToNode(ScopedNodeId scopedNodeId, OnConnectedAction onConnectedAction)
{
    if ((scopedNodeId.GetFabricIndex() == kUndefinedFabricIndex) ||
        (scopedNodeId.GetNodeId() == kUndefinedNodeId))
    {
        ChipLogError(DeviceLayer, "Invalid node location!");
        return;
    }

    // Set the action to take once connection is successfully established
    //mOnConnectedAction = onConnectedAction;

    ChipLogDetail(DeviceLayer, "Establishing session to provider node ID 0x" ChipLogFormatX64 " on fabric index %d",
                  ChipLogValueX64(scopedNodeId.GetNodeId()), scopedNodeId.GetFabricIndex());

    //mCASESessionManager->FindOrEstablishSession(GetProviderScopedId(), &mOnConnectedCallback, &mOnConnectionFailureCallback);
}
