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

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

namespace {
}

DeviceDatastoreCache DeviceDatastoreCache::sInstance;

void DeviceDatastoreCache::Init()
{
    mInitialized    = true;
}


CHIP_ERROR DeviceDatastoreCache::AddDevice(NodeId nodeIdValue, chip::Optional<chip::CharSpan> friendlyName)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrReturnError(mDeviceDataStoreCache.size() < kMaxDevices, CHIP_ERROR_NO_MEMORY);

    mDeviceDataStoreCache.push_back(DeviceEntry(nodeIdValue, friendlyName));

    return err;
}

DeviceEntry* DeviceDatastoreCache::GetDevice(NodeId nodeIdValue)
{
    for (auto & deviceEntry : mDeviceDataStoreCache)
    {
        if (deviceEntry.nodeId == nodeIdValue)
        {
		return &deviceEntry;
        }
    }

    return nullptr;
}

void DeviceDatastoreCache::PrintDevices()
{
    ChipLogProgress(JointFabric, "DeviceDatastoreCache contents: ");

    for (auto & deviceEntry : mDeviceDataStoreCache)
    {
        ChipLogProgress(JointFabric, "NodeID: %lu, friendlyName: %s", deviceEntry.nodeId, deviceEntry.friendlyName.data());
        ChipLogProgress(JointFabric, "VendorName: %s, ProductName: %s", deviceEntry.vendorName.data(), deviceEntry.productName.data());
        ChipLogProgress(JointFabric, "Reachable: %d, HW-Version: %d, SW-Version: %d, On: %d", deviceEntry.reachable,
                        deviceEntry.hardwareVersion, deviceEntry.softwareVersion, deviceEntry.on);
    }
}
