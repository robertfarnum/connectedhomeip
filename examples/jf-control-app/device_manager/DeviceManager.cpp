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
using namespace chip::app;
using namespace chip::app::Clusters;

namespace {

static constexpr char kAlpha[] = "alpha";
static constexpr char kBeta[]  = "beta";

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

void DeviceManager::HandleOnResponse(const app::ConcreteDataAttributePath & path, NodeId remotePeerNodeId)
{
    StringBuilder<kMaxCommandSize> commandBuilder;

    if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::NodeLabel::Id))
    {
        nodeIdToRefreshFriendlyName = remotePeerNodeId;

        if (jfAdminAppCommissioned)
        {
            commandBuilder.Add("basicinformation read node-label ");
            commandBuilder.AddFormat("%lu %d ", nodeIdToRefreshFriendlyName, kRootEndpointId);
            PushCommand(commandBuilder.c_str());
        }
    }
}

void DeviceManager::HandleOnAttributeData(const app::ConcreteDataAttributePath & path, TLV::TLVReader * data)
{
    CHIP_ERROR error = CHIP_NO_ERROR;

    uint8_t friendlyNameBuffer[16] = {0};
    MutableByteSpan friendlyNameMutableByteSpan{ friendlyNameBuffer };
    CharSpan friendlyNameCharSpan;
    StringBuilder<kMaxCommandSize> commandBuilder;
    size_t size_to_copy = 0;

    if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::NodeLabel::Id))
    {
        if (jfAdminAppCommissioned && (nodeIdToRefreshFriendlyName != kUndefinedNodeId))
        {
            error = DataModel::Decode(*data, friendlyNameCharSpan);
            if (CHIP_NO_ERROR == error)
            {
                size_to_copy = friendlyNameCharSpan.size() > 15 ? 15 : friendlyNameCharSpan.size();
                memcpy (friendlyNameMutableByteSpan.data(), friendlyNameCharSpan.data(), size_to_copy);

                commandBuilder.Add("jointfabricdatastore update-node ");
                commandBuilder.AddFormat("%lu %s %lu %d",nodeIdToRefreshFriendlyName, friendlyNameMutableByteSpan.data(),
                                         jfAdminAppNodeId, kRootEndpointId);
                PushCommand(commandBuilder.c_str());
            }

            nodeIdToRefreshFriendlyName = kUndefinedNodeId;
        }
    }
}

const char* DeviceManager::GetCurrentCommissioner()
{
    if (!jfOnboarded)
    {
        return kAlpha;
    }

    return kBeta;
}

void DeviceManager::SetJfOnboarded(uint64_t nodeId)
{
    jfOnboarded = true;
    jfAdminAppNodeId = nodeId;
}
