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
#include <platform/CHIPDeviceLayer.h>

struct DeviceEntry
{
public:
    DeviceEntry(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        Set(nodeIdValue, label);
    }

    DeviceEntry(const DeviceEntry & op) { *this = op; }

    DeviceEntry & operator=(const DeviceEntry & op)
    {
        Set(op.nodeId, MakeOptional(op.friendlyName));
        SetVendorName(MakeOptional(op.vendorName));
        SetProductName(MakeOptional(op.productName));
        SetReachable(op.reachable);
        SetHardwareVersion(MakeOptional(op.hardwareVersionString));
        SetSoftwareVersion(MakeOptional(op.softwareVersionString));
        SetOn(op.on);
        SetOnOffSubscriptionEstablished(op.OnOffSubscriptionEstablished);
        SetType(op.type);

        return *this;
    }

    void Set(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        this->nodeId = nodeIdValue;
        SetFriendlyName(label);
    }

    void SetCharSpan(chip::CharSpan * spanToSet, char* charSpanBuffer, size_t bufSize,
                     chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        if (label.HasValue())
        {
            memset(charSpanBuffer, 0, bufSize);
            if (label.Value().size() > bufSize)
            {
                memcpy(charSpanBuffer, label.Value().data(), bufSize);
                *spanToSet = chip::CharSpan(charSpanBuffer, bufSize);
            }
            else
            {
                memcpy(charSpanBuffer, label.Value().data(), label.Value().size());
                *spanToSet = chip::CharSpan(charSpanBuffer, label.Value().size());
            }
        }
        else
        {
            *spanToSet = chip::CharSpan();
        }

        /* TODO: call JSON callback here */
    }

    void SetFriendlyName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        SetCharSpan(&(this->friendlyName), this->mFriendlyNameBuffer, kFriendlyNameMaxSize, label);
    }

    void SetVendorName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        SetCharSpan(&(this->vendorName), this->mVendorNameBuffer, kVendorNameMaxSize, label);
    }

    void SetProductName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        SetCharSpan(&(this->productName), this->mProductNameBuffer, kProductNameMaxSize, label);
    }

    void SetHardwareVersion(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        SetCharSpan(&(this->hardwareVersionString), this->mHardwareVersionBuffer, kHardwareVersionMaxSize, label);
    }

    void SetSoftwareVersion(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        SetCharSpan(&(this->softwareVersionString), this->mSoftwareVersionBuffer, kSoftwareVersionMaxSize, label);
    }

    void SetReachable(bool value)
    {
        if (this->reachable != value)
        {
            this->reachable = value;
            /* TODO: call JSON callback here */
        }
    }

    void SetOn(bool value)
    {
        if (this->on != value)
        {
            this->on = value;
            /* TODO: call JSON callback here */
        }
    }

    chip::NodeId GetNodeId()
    {
        return this->nodeId;
    }

    chip::CharSpan GetFriendlyName()
    {
        return this->friendlyName;
    }

    chip::CharSpan GetVendorName()
    {
        return this->vendorName;
    }

    chip::CharSpan GetProductName()
    {
        return this->productName;
    }

    bool GetReachable()
    {
        return this->reachable = true;
    }

    chip::CharSpan GetHardwareVersionString()
    {
        return this->hardwareVersionString;
    }

    chip::CharSpan GetSoftwareVersionString()
    {
	    return this->softwareVersionString;
    }

    bool GetOn()
    {
	    return this->on;
    }

    bool GetOnOffSubscriptionEstablished()
    {
        return this->OnOffSubscriptionEstablished;
    }

    uint8_t GetType()
    {
        return this->type;
    }

    void SetOnOffSubscriptionEstablished(bool value)
    {
        if (this->OnOffSubscriptionEstablished != value)
        {
            this->OnOffSubscriptionEstablished = value;
            /* TODO: call JSON callback here */
        }
    }

    void SetType(uint8_t value)
    {
        if (this->type != value)
        {
            this->type = value;
            /* TODO: call JSON callback here */
        }
    }

private:
    chip::NodeId nodeId = chip::kUndefinedNodeId;
    chip::CharSpan friendlyName;
    chip::CharSpan vendorName;
    chip::CharSpan productName;
    bool reachable = true; // fixed value for now
    chip::CharSpan hardwareVersionString;
    chip::CharSpan softwareVersionString;
    bool on = false; // valid only for on-off devices.
    bool OnOffSubscriptionEstablished = false; // valid only for on-off devices.
    uint8_t type = 0; // 1 for admin devices

    static constexpr size_t kFriendlyNameMaxSize = 32u;
    char mFriendlyNameBuffer[kFriendlyNameMaxSize];

    static constexpr size_t kVendorNameMaxSize = 32u;
    char mVendorNameBuffer[kVendorNameMaxSize];

    static constexpr size_t kProductNameMaxSize = 32u;
    char mProductNameBuffer[kProductNameMaxSize];

    static constexpr size_t kHardwareVersionMaxSize = 32u;
    char mHardwareVersionBuffer[kHardwareVersionMaxSize];

    static constexpr size_t kSoftwareVersionMaxSize = 32u;
    char mSoftwareVersionBuffer[kSoftwareVersionMaxSize];
};

class DeviceDatastoreCache
{
public:
    DeviceDatastoreCache() = default;

    void Init();

    CHIP_ERROR AddDevice(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> friendlyName = chip::NullOptional);
    CHIP_ERROR RemoveDevice(chip::NodeId nodeIdValue);

    DeviceEntry* GetDevice(chip::NodeId nodeIdValue);
    void PrintDevices();

    const std::vector<DeviceEntry> & GetDeviceDatastoreCache()
    {
        return mDeviceDataStoreCache;
    }

private:
    friend DeviceDatastoreCache & DeviceDatastoreCacheInstance();

    static DeviceDatastoreCache sInstance;
    bool mInitialized     = false;

    static constexpr size_t kMaxDevices = 32;

    std::vector<DeviceEntry> mDeviceDataStoreCache;
};

inline DeviceDatastoreCache & DeviceDatastoreCacheInstance()
{
    if (!DeviceDatastoreCache::sInstance.mInitialized)
    {
        DeviceDatastoreCache::sInstance.Init();
    }
    return DeviceDatastoreCache::sInstance;
}
