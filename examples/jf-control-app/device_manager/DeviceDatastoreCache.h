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
    virtual void Updated(DeviceEntry * deviceEntry) = 0;
    virtual ~DeviceEntryListener()                  = default;
};

struct DeviceEntry
{
public:
    chip::NodeId nodeId = chip::kUndefinedNodeId;
    chip::CharSpan friendlyName;
    chip::CharSpan vendorName;
    chip::CharSpan productName;
    bool reachable           = false;
    uint16_t hardwareVersion = 0;
    uint32_t softwareVersion = 0;
    bool on                  = false; // valid only for on-off devices.

    DeviceEntry(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> label = chip::NullOptional) { Set(nodeIdValue, label); }

    DeviceEntry(const DeviceEntry & op) { *this = op; }

    DeviceEntry & operator=(const DeviceEntry & op)
    {
        Set(op.nodeId, MakeOptional(op.friendlyName));
        SetVendorName(MakeOptional(op.vendorName));
        SetProductName(MakeOptional(op.productName));

        SetReachable(op.reachable);
        SetHardwareVersion(op.hardwareVersion);
        SetSoftwareVersion(op.softwareVersion);
        SetOn(op.on);

        return *this;
    }

    void Set(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        this->nodeId = nodeIdValue;
        SetFriendlyName(label);
    }

    void SetFriendlyName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        if (label.HasValue())
        {
            memset(mFriendlyNameBuffer, 0, sizeof(mFriendlyNameBuffer));
            if (label.Value().size() > sizeof(mFriendlyNameBuffer))
            {
                memcpy(mFriendlyNameBuffer, label.Value().data(), sizeof(mFriendlyNameBuffer));
                this->friendlyName = chip::CharSpan(mFriendlyNameBuffer, sizeof(mFriendlyNameBuffer));
            }
            else
            {
                memcpy(mFriendlyNameBuffer, label.Value().data(), label.Value().size());
                this->friendlyName = chip::CharSpan(mFriendlyNameBuffer, label.Value().size());
            }
        }
        else
        {
            this->friendlyName = chip::CharSpan();
        }
    }

    void SetVendorName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        if (label.HasValue())
        {
            memset(mVendorNameBuffer, 0, sizeof(mVendorNameBuffer));
            if (label.Value().size() > sizeof(mVendorNameBuffer))
            {
                memcpy(mVendorNameBuffer, label.Value().data(), sizeof(mVendorNameBuffer));
                this->vendorName = chip::CharSpan(mVendorNameBuffer, sizeof(mVendorNameBuffer));
            }
            else
            {
                memcpy(mVendorNameBuffer, label.Value().data(), label.Value().size());
                this->vendorName = chip::CharSpan(mVendorNameBuffer, label.Value().size());
            }
        }
        else
        {
            this->vendorName = chip::CharSpan();
        }
    }

    void SetProductName(chip::Optional<chip::CharSpan> label = chip::NullOptional)
    {
        if (label.HasValue())
        {
            memset(mProductNameBuffer, 0, sizeof(mProductNameBuffer));
            if (label.Value().size() > sizeof(mProductNameBuffer))
            {
                memcpy(mProductNameBuffer, label.Value().data(), sizeof(mProductNameBuffer));
                this->productName = chip::CharSpan(mProductNameBuffer, sizeof(mProductNameBuffer));
            }
            else
            {
                memcpy(mProductNameBuffer, label.Value().data(), label.Value().size());
                this->productName = chip::CharSpan(mProductNameBuffer, label.Value().size());
            }
        }
        else
        {
            this->productName = chip::CharSpan();
        }
    }

    void SetReachable(bool value) { this->reachable = value; }

    void SetHardwareVersion(uint16_t value) { this->hardwareVersion = value; }

    void SetSoftwareVersion(uint32_t value) { this->softwareVersion = value; }

    void SetOn(bool value) { this->on = value; }

    void AddListener(DeviceEntryListener * listener) { listeners.push_back(listener); }

private:
    static constexpr size_t kFriendlyNameMaxSize = 32u;
    char mFriendlyNameBuffer[kFriendlyNameMaxSize];

    static constexpr size_t kVendorNameMaxSize = 32u;
    char mVendorNameBuffer[kVendorNameMaxSize];

    static constexpr size_t kProductNameMaxSize = 32u;
    char mProductNameBuffer[kProductNameMaxSize];

    std::vector<DeviceEntryListener *> listeners;

    void triggerListeners()
    {
        for (const auto & listener : listeners)
        {
            listener->Updated(this);
        }
    }
};

class DeviceDatastoreCacheListener
{
public:
    virtual void DeviceAdded(chip::NodeId nodeIdValue)   = 0;
    virtual void DeviceRemoved(chip::NodeId nodeIdValue) = 0;
    virtual ~DeviceDatastoreCacheListener()              = default;
};

class DeviceDatastoreCache
{
public:
    DeviceDatastoreCache() = default;

    void Init();

    CHIP_ERROR AddDevice(chip::NodeId nodeIdValue, chip::Optional<chip::CharSpan> friendlyName = chip::NullOptional);
    CHIP_ERROR RemoveDevice(chip::NodeId nodeIdValue);
    DeviceEntry * GetDevice(chip::NodeId nodeIdValue);
    std::vector<DeviceEntry> GetDevices();
    void PrintDevices();

    void AddListener(DeviceDatastoreCacheListener * listener) { listeners.push_back(listener); }

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
        for (const auto & listener : listeners)
        {
            listener->DeviceRemoved(nodeIdValue);
        }
    }

    // Trigger Device Added Listeners
    void triggerDeviceAddedListeners(chip::NodeId nodeIdValue)
    {
        for (const auto & listener : listeners)
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
