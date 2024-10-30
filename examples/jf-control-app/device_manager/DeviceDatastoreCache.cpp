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

#include "DeviceDatastoreCache.h"

#include <algorithm>
#include <vector>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

namespace {}

DeviceDatastoreCache DeviceDatastoreCache::sInstance;

void DeviceDatastoreCache::Init()
{
    mInitialized = true;
}

CHIP_ERROR DeviceDatastoreCache::AddDevice(NodeId nodeIdValue, chip::Optional<chip::CharSpan> friendlyName)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrReturnError(mDeviceDatastoreCache.size() < kMaxDevices, CHIP_ERROR_NO_MEMORY);

    if (GetDevice(nodeIdValue) != nullptr)
    {
        ChipLogError(JointFabric, "AddDevice found: ");
        PrintDevices();
        return CHIP_ERROR_NOT_FOUND;
    }

    mDeviceDatastoreCache.push_back(DeviceEntry(nodeIdValue, friendlyName));

    triggerDeviceAddedListeners(nodeIdValue);

    return err;
}

CHIP_ERROR DeviceDatastoreCache::RemoveDevice(NodeId nodeIdValue)
{
    for (auto it = mDeviceDatastoreCache.begin(); it != mDeviceDatastoreCache.end(); ++it)
    {
        if (it->GetNodeId() == nodeIdValue)
        {
            mDeviceDatastoreCache.erase(it);

            triggerDeviceRemovedListeners(nodeIdValue);

            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

DeviceEntry * DeviceDatastoreCache::GetDevice(NodeId nodeIdValue)
{
    for (auto & deviceEntry : mDeviceDatastoreCache)
    {
        if (deviceEntry.GetNodeId() == nodeIdValue)
        {
            return &deviceEntry;
        }
    }

    return nullptr;
}

void DeviceDatastoreCache::PrintDevices()
{
    ChipLogProgress(JointFabric, "DeviceDatastoreCache contents: ");

    for (auto & deviceEntry : mDeviceDatastoreCache)
    {
        ChipLogProgress(JointFabric, "NodeID: %lu, friendlyName: %s", deviceEntry.GetNodeId(),
                        deviceEntry.GetFriendlyName().data());
        ChipLogProgress(JointFabric, "VendorName: %s, ProductName: %s", deviceEntry.GetVendorName().data(),
                        deviceEntry.GetProductName().data());
        ChipLogProgress(JointFabric, "Reachable: %d, HW-Version: %s, SW-Version: %s, On: %d", deviceEntry.GetReachable(),
                        deviceEntry.GetHardwareVersionString().data(), deviceEntry.GetSoftwareVersionString().data(),
                        deviceEntry.GetOn());
        ChipLogProgress(JointFabric, "Type:%d, OnOffSubscriptionEstablished: %d", deviceEntry.GetType(),
                        deviceEntry.GetOnOffSubscriptionEstablished());
    }
}
