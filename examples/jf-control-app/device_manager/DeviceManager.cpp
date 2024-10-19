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

#include "DeviceManager.h"

#include <commands/common/CHIPCommand.h>
#include <commands/interactive/InteractiveCommands.h>
#include <lib/support/StringBuilder.h>

using namespace chip;
using namespace chip::app::Clusters;

namespace {

} // namespace

// Define the static member
DeviceManager DeviceManager::sInstance;

void DeviceManager::Init()
{
    mInitialized    = true;
}

void DeviceManager::HandleCommissioningComplete(chip::NodeId nodeId)
{
    StringBuilder<kMaxCommandSize> commandBuilder;
    if (jfAdminAppCommissioned)
    {
        commandBuilder.Add("jointfabricdatastore add-pending-node ");
        commandBuilder.AddFormat("%lu FriendlyName %lu %d ", nodeId, jfAdminAppNodeId, kRootEndpointId);
        PushCommand(commandBuilder.c_str());
    
        commandBuilder.Reset();
        commandBuilder.Add("jointfabricdatastore refresh-node ");
        commandBuilder.AddFormat("%lu %lu %d ", nodeId, jfAdminAppNodeId, kRootEndpointId);
        PushCommand(commandBuilder.c_str());
    }
    else
    {
    	jfAdminAppNodeId = nodeId;
        jfAdminAppCommissioned = true;
    }
}

void DeviceManager::HandleCommandResponse(const app::ConcreteCommandPath & path, TLV::TLVReader & data)
{
    if (path.mClusterId == JointFabricDatastore::Id)
    {
        ChipLogProgress(NotSpecified, "Command Response received for JointFabricDatastore::Id");
    }
}
