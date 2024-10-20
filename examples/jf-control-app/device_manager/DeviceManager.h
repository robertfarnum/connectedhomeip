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

#include <app-common/zap-generated/cluster-objects.h>
#include <commands/pairing/PairingCommand.h>
#include <platform/CHIPDeviceLayer.h>

class DeviceManager
{
public:
    DeviceManager() = default;

    void Init();

    /* callback for commissioning complete */
    void HandleCommissioningComplete(chip::NodeId nodeId);

    /* callback for attribute write */
    void HandleOnResponse(const chip::app::ConcreteDataAttributePath & path, chip::NodeId remotePeerNodeId);

    /* callback for attribute read */
    void HandleOnAttributeData(const chip::app::ConcreteDataAttributePath & path, chip::TLV::TLVReader * data);

private:
    friend DeviceManager & DeviceMgr();
    
    static DeviceManager sInstance;
    bool mInitialized     = false;
    bool jfAdminAppCommissioned = false;
    chip::NodeId jfAdminAppNodeId = chip::kUndefinedNodeId;

    chip::NodeId nodeIdToRefreshFriendlyName = chip::kUndefinedNodeId;
};

/**
 * Returns the public interface of the DeviceManager singleton object.
 *
 * Applications should use this to access features of the DeviceManager
 * object.
 */
inline DeviceManager & DeviceMgr()
{
    if (!DeviceManager::sInstance.mInitialized)
    {
        DeviceManager::sInstance.Init();
    }
    return DeviceManager::sInstance;
}
