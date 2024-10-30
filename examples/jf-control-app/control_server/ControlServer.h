#include <json/json.h>
#include <lib/core/CHIPError.h>
#include <lib/support/logging/CHIPLogging.h>

#include "device_manager/DeviceDatastoreCache.h"

class ControlServer : public DeviceDatastoreCacheListener, DeviceEntryListener
{
public:
    ControlServer();
    ~ControlServer();

        void DeviceAdded(chip::NodeId nodeId) override
    {
        ChipLogProgress(NotSpecified, "DeviceAdded(%lu)", nodeId);

        DeviceEntry * deviceEntry = DeviceDatastoreCacheInstance().GetDevice(nodeId);
        if (deviceEntry == nullptr)
        {
            ChipLogError(NotSpecified, "DeviceUpdate() Invalid NodeId = %lu", nodeId);
        }

        addDevice(*deviceEntry);
    }

    void DeviceRemoved(chip::NodeId nodeId) override
    {
        ChipLogProgress(NotSpecified, "DeviceRemoved(%lu)", nodeId);

        DeviceEntry * deviceEntry = DeviceDatastoreCacheInstance().GetDevice(nodeId);
        if (deviceEntry == nullptr)
        {
            ChipLogError(NotSpecified, "DeviceUpdate() Invalid NodeId = %lu", nodeId);
        }

        removeDevice(*deviceEntry);
    }

    void DeviceUpdated(chip::NodeId nodeId) override
    {
        ChipLogProgress(NotSpecified, "Device Updated = %lu", nodeId);

        DeviceEntry * deviceEntry = DeviceDatastoreCacheInstance().GetDevice(nodeId);
        if (deviceEntry == nullptr)
        {
            ChipLogError(NotSpecified, "DeviceUpdate() Invalid NodeId = %lu", nodeId);
            return;
        }

        Json::Value * device = findDevice(nodeId);
        if (device == NULL)
        {
            ChipLogError(NotSpecified, "DeviceUpdate() Failed to find device = %lu", nodeId);
            return;
        }

        updateDevice(*device, *deviceEntry);
    }

    Json::Value HandleMethod(std::string method, Json::Value data);

private:
    void updateOnOff(Json::Value & device, bool on);
    void updateFriendlyName(Json::Value & device, std::string friendlyName);

    // Method handlers
    Json::Value handleDeleteDevice(Json::Value data);
    Json::Value handleControlDevice(Json::Value data);
    Json::Value handleGetDevices(Json::Value data);
    Json::Value handleCommissionDevice(Json::Value data);
    Json::Value handleCommissionAdminDevice(Json::Value data);
    Json::Value handleOpenCommissioningWindow(Json::Value data);

    Json::Value * findDevice(chip::NodeId nodeId);
    void updateDevice(Json::Value & device, DeviceEntry & deviceEntry);
    void addDevice(DeviceEntry & deviceEntry);
    void removeDevice(DeviceEntry & deviceEntry);

    chip::NodeId nextNodeId = 20;
    Json::Value devices     = Json::Value(Json::arrayValue);

    std::map<std::string, Json::Value (ControlServer::*)(Json::Value)> methodHandlers = {
        { "OpenCommissioningWindow", &ControlServer::handleOpenCommissioningWindow },
        { "CommissionDevice", &ControlServer::handleCommissionDevice },
        { "CommissionAdminDevice", &ControlServer::handleCommissionAdminDevice },
        { "GetDevices", &ControlServer::handleGetDevices },
        { "ControlDevice", &ControlServer::handleControlDevice },
        { "DeleteDevice", &ControlServer::handleDeleteDevice }
    };
};
