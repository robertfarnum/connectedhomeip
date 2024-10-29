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

#include "DeviceDatastoreCache.h"
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
    mInitialized = true;
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
        jfAdminAppNodeId       = nodeId;
        jfAdminAppCommissioned = true;

        /* establish a subscription to JFA DS Node-List*/
        commandBuilder.Reset();
        commandBuilder.Add("jointfabricdatastore subscribe node-list ");
        commandBuilder.AddFormat("1 180 %lu %d", jfAdminAppNodeId, kRootEndpointId);
        PushCommand(commandBuilder.c_str());
    }

    AddNewNodeInDatastoreCache(nodeId);
}

void DeviceManager::HandleOnResponse(const app::ConcreteDataAttributePath & path, NodeId remotePeerNodeId)
{
    StringBuilder<kMaxCommandSize> commandBuilder;

    if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::NodeLabel::Id))
    {
        if (jfAdminAppCommissioned)
        {
            commandBuilder.Add("basicinformation read node-label ");
            commandBuilder.AddFormat("%lu %d ", remotePeerNodeId, kRootEndpointId);
            PushCommand(commandBuilder.c_str());
        }
    }
}

void DeviceManager::HandleOnAttributeData(const app::ConcreteDataAttributePath & path, TLV::TLVReader * data, NodeId destinationId)
{
    CHIP_ERROR error = CHIP_NO_ERROR;
    StringBuilder<kMaxCommandSize> commandBuilder;

    chip::TLV::TLVReader reader;
    reader.Init(*data);

    if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::NodeLabel::Id))
    {
        uint8_t friendlyNameBuffer[33] = { 0 }; // one extra byte required for the null terminator
        CharSpan friendlyNameCharSpan;
        size_t size_to_copy = 0;
        error               = DataModel::Decode(reader, friendlyNameCharSpan);

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned && friendlyNameCharSpan.size())
        {
            size_to_copy = friendlyNameCharSpan.size() > 32 ? 32 : friendlyNameCharSpan.size();
            memcpy(friendlyNameBuffer, friendlyNameCharSpan.data(), size_to_copy);

            DeviceEntry * deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetFriendlyName(MakeOptional(friendlyNameCharSpan));
            }

            commandBuilder.Add("jointfabricdatastore update-node ");
            commandBuilder.AddFormat("%lu '%s' %lu %d", destinationId, friendlyNameBuffer, jfAdminAppNodeId, kRootEndpointId);
            PushCommand(commandBuilder.c_str());
        }
    }
    else if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::VendorName::Id))
    {
        CharSpan value;
        error = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetVendorName(MakeOptional(value));
            }
        }
    }
    else if ((path.mClusterId == BasicInformation::Id) && (path.mAttributeId == BasicInformation::Attributes::ProductName::Id))
    {
        CharSpan value;
        error = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetProductName(MakeOptional(value));
            }
        }
    }
    else if ((path.mClusterId == BasicInformation::Id) &&
             (path.mAttributeId == BasicInformation::Attributes::HardwareVersionString::Id))
    {
        CharSpan value;
        error = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetHardwareVersion(MakeOptional(value));
            }
        }
    }
    else if ((path.mClusterId == BasicInformation::Id) &&
             (path.mAttributeId == BasicInformation::Attributes::SoftwareVersionString::Id))
    {
        CharSpan value;
        error = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetSoftwareVersion(MakeOptional(value));
            }
        }
    }
    else if ((path.mClusterId == OnOff::Id) && (path.mAttributeId == OnOff::Attributes::OnOff::Id))
    {
        bool value = false;
        error      = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetOn(value);
            }

            if (!deviceEntry->GetOnOffSubscriptionEstablished())
            {
                /* establish subscription to on-off attribute */
                commandBuilder.Add("onoff subscribe on-off 1 180 ");
                commandBuilder.AddFormat("%lu %d ", destinationId, kRootEndpointId + 1);
                PushCommand(commandBuilder.c_str());

                deviceEntry->SetOnOffSubscriptionEstablished(true);
            }
        }
    }
    else if ((path.mClusterId == JointFabricDatastore::Id) &&
             (path.mAttributeId == JointFabricDatastore::Attributes::ClusterRevision::Id))
    {
        uint16_t value = 0;
        error          = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            deviceEntry = DeviceDatastoreCacheInstance().GetDevice(destinationId);
            if (deviceEntry)
            {
                deviceEntry->SetType(1);
            }
        }
    }
    else if ((path.mClusterId == JointFabricDatastore::Id) && (path.mAttributeId == JointFabricDatastore::Attributes::NodeList::Id))
    {
        chip::app::DataModel::DecodableList<
            chip::app::Clusters::JointFabricDatastore::Structs::DatastoreNodeInformationEntry::DecodableType>
            value;
        error = chip::app::DataModel::Decode(reader, value);
        DeviceEntry * deviceEntry;

        if ((CHIP_NO_ERROR == error) && jfAdminAppCommissioned)
        {
            auto iter = value.begin();
            while (iter.Next())
            {
                NodeId nodeId         = iter.GetValue().nodeID;
                CharSpan friendlyName = iter.GetValue().friendlyName;

                deviceEntry = DeviceDatastoreCacheInstance().GetDevice(nodeId);
                if (deviceEntry && (deviceEntry->GetFriendlyName().data_equal(friendlyName) == false))
                {
                    deviceEntry->SetFriendlyName(MakeOptional(friendlyName));
                }
                else if (!deviceEntry)
                {
                    AddNewNodeInDatastoreCache(nodeId, MakeOptional(friendlyName));
                }
            }
        }
    }
}

void DeviceManager::TriggerDatastoreCachePopulation(NodeId nodeId)
{
    StringBuilder<kMaxCommandSize> commandBuilder;

    commandBuilder.Add("basicinformation read node-label ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("basicinformation read vendor-name ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("basicinformation read product-name ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("basicinformation read hardware-version-string ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("basicinformation read software-version-string ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("jointfabricdatastore read cluster-revision ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());

    commandBuilder.Reset();
    commandBuilder.Add("onoff read on-off ");
    commandBuilder.AddFormat("%lu %d ", nodeId, kRootEndpointId + 1);
    PushCommand(commandBuilder.c_str());
}

void DeviceManager::AddNewNodeInDatastoreCache(chip::NodeId nodeId, chip::Optional<chip::CharSpan> friendlyName)
{
    StringBuilder<kMaxCommandSize> commandBuilder;

    /* add device in the DS cache */
    DeviceDatastoreCacheInstance().AddDevice(nodeId, friendlyName);

    TriggerDatastoreCachePopulation(nodeId);
}

const char * DeviceManager::GetCurrentCommissioner()
{
    if (!jfOnboarded)
    {
        return kAlpha;
    }

    return kBeta;
}

void DeviceManager::SetJfOnboarded(uint64_t nodeId)
{
    StringBuilder<kMaxCommandSize> commandBuilder;

    jfOnboarded = true;
    /* stop all subscriptions */
    commandBuilder.Reset();
    commandBuilder.Add("subscriptions shutdown-all");
    PushCommand(commandBuilder.c_str());

    jfAdminAppNodeId = nodeId;

    /* establish a subscription to Anchor JFA DS Node-List*/
    commandBuilder.Reset();
    commandBuilder.Add("jointfabricdatastore subscribe node-list ");
    commandBuilder.AddFormat("1 180 %lu %d", jfAdminAppNodeId, kRootEndpointId);
    PushCommand(commandBuilder.c_str());
}
