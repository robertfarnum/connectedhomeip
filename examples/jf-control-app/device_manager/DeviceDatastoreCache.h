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

struct DeviceEntry;

class DeviceEntryListener
{
public:
    virtual void DeviceUpdated(chip::NodeId nodeId) = 0;
    virtual ~DeviceEntryListener()                  = default;
};

struct DeviceEntry
{
public:
    DeviceEntry(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> label = chip::NullOptional) { Set(nodeIdValue, label); }

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

    void SetCharSpan(chip::CharSpan * spanToSet, char * charSpanBuffer, size_t bufSize,
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

        triggerListeners();
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

            triggerListeners();
        }
    }

    void SetOn(bool value)
    {
        if (this->on != value)
        {
            this->on = value;

            triggerListeners();
        }
    }

    chip::NodeId GetNodeId() { return this->nodeId; }

    chip::CharSpan GetFriendlyName() { return this->friendlyName; }

    chip::CharSpan GetVendorName() { return this->vendorName; }

    chip::CharSpan GetProductName() { return this->productName; }

    bool GetReachable() { return this->reachable = true; }

    chip::CharSpan GetHardwareVersionString() { return this->hardwareVersionString; }

    chip::CharSpan GetSoftwareVersionString() { return this->softwareVersionString; }

    bool GetOn() { return this->on; }

    bool GetOnOffSubscriptionEstablished() { return this->OnOffSubscriptionEstablished; }

    uint8_t GetType() { return this->type; }

    void SetOnOffSubscriptionEstablished(bool value)
    {
        if (this->OnOffSubscriptionEstablished != value)
        {
            this->OnOffSubscriptionEstablished = value;

            triggerListeners();
        }
    }

    void SetType(uint8_t value)
    {
        if (this->type != value)
        {
            this->type = value;

            triggerListeners();
        }
    }

    void AddListener(DeviceEntryListener * listener) { listeners.push_back(listener); }
    void RemoveListener(DeviceEntryListener * listener)
    {
        for (size_t i = 0; i < listeners.size(); i++)
        {
            if (listeners[i] == listener)
            {
                listeners.erase(listeners.begin() + i);
                break;
            }
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
    bool on                           = false; // valid only for on-off devices.
    bool OnOffSubscriptionEstablished = false; // valid only for on-off devices.
    uint8_t type                      = 0;     // 1 for admin devices

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

    std::vector<DeviceEntryListener *> listeners;

    void triggerListeners()
    {
        for (const auto & listener : listeners)
        {
            listener->DeviceUpdated(nodeId);
        }
    }
};

class DeviceDatastoreCacheListener
{
public:
    virtual void DeviceAdded(chip::NodeId nodeId)   = 0;
    virtual void DeviceRemoved(chip::NodeId nodeId) = 0;
    virtual ~DeviceDatastoreCacheListener()         = default;
};

class DeviceDatastoreCache
{
public:
    DeviceDatastoreCache() = default;

    void Init();

    CHIP_ERROR AddDevice(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> friendlyName = chip::NullOptional);
    CHIP_ERROR RemoveDevice(chip::NodeId nodeIdValue);

    DeviceEntry * GetDevice(chip::NodeId nodeIdValue);
    void PrintDevices();

    const std::vector<DeviceEntry> & GetDeviceDatastoreCache() { return mDeviceDatastoreCache; }

    void AddListener(DeviceDatastoreCacheListener * listener) { listeners.push_back(listener); }
    void RemoveListener(DeviceDatastoreCacheListener * listener)
    {
        for (size_t i = 0; i < listeners.size(); i++)
        {
            if (listeners[i] == listener)
            {
                listeners.erase(listeners.begin() + i);
                break;
            }
        }
    }

    chip::NodeId CheckForRemovalInDatastoreCache(std::vector<chip::NodeId> & dsNodeLists);

private:
    friend DeviceDatastoreCache & DeviceDatastoreCacheInstance();

    static DeviceDatastoreCache sInstance;
    bool mInitialized = false;

    static constexpr size_t kMaxDevices = 32;

    std::vector<DeviceEntry> mDeviceDatastoreCache;
    std::vector<DeviceDatastoreCacheListener *> listeners;

    // Trigger Device Removed Listeners
    void triggerDeviceRemovedListeners(chip::NodeId nodeIdValue)
    {
        for (DeviceDatastoreCacheListener * listener : listeners)
        {
            listener->DeviceRemoved(nodeIdValue);
        }
    }

    // Trigger Device Added Listeners
    void triggerDeviceAddedListeners(chip::NodeId nodeIdValue)
    {
        for (DeviceDatastoreCacheListener * listener : listeners)
        {
            listener->DeviceAdded(nodeIdValue);
        }
    }
};

inline DeviceDatastoreCache & DeviceDatastoreCacheInstance()
{
    if (!DeviceDatastoreCache::sInstance.mInitialized)
    {
        DeviceDatastoreCache::sInstance.Init();
    }
    return DeviceDatastoreCache::sInstance;
}
