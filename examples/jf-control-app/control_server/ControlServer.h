#include <json/json.h>
#include <lib/core/CHIPError.h>
#include <lib/support/logging/CHIPLogging.h>

#include "device_manager/DeviceDatastoreCache.h"

class ControlServer : DeviceDatastoreCacheListener, DeviceEntryListener
{
public:
    ControlServer();

    virtual void DeviceAdded(chip::NodeId nodeId)
    {
        ChipLogProgress(NotSpecified, "DeviceAdded(%lu)", nodeId);

        DeviceEntry * deviceEntry = DeviceDatastoreCacheInstance().GetDevice(nodeId);
        addDevice(deviceEntry);
    }

    virtual void DeviceRemoved(chip::NodeId nodeId)
    {
        ChipLogProgress(NotSpecified, "DeviceRemoved(%lu)", nodeId);

        removeDevice(nodeId);
    }

    virtual void DeviceUpdated(DeviceEntry * deviceEntry)
    {
        if (deviceEntry == NULL)
        {
            return;
        }

        ChipLogProgress(NotSpecified, "Device Updated = %lu", deviceEntry->GetNodeId());

        Json::Value * device = findDevice(deviceEntry->GetNodeId());

        if (device != NULL)
        {
            updateDevice(device, deviceEntry);
        }
    }

    Json::Value HandleMethod(std::string method, Json::Value data);

private:
    void updateOnOff(Json::Value * device, bool on);
    void updateFriendlyName(Json::Value * device, std::string friendlyName);

    // Method handlers
    Json::Value handleDeleteDevice(Json::Value data);
    Json::Value handleControlDevice(Json::Value data);
    Json::Value handleGetDevices(Json::Value data);
    Json::Value handleCommissionDevice(Json::Value data);
    Json::Value handleCommissionAdminDevice(Json::Value data);
    Json::Value handleOpenCommissioningWindow(Json::Value data);

    Json::Value * findDevice(chip::NodeId nodeId);
    void updateDevice(Json::Value * device, DeviceEntry * deviceEntry);
    void addDevice(DeviceEntry * deviceEntry);
    void removeDevice(chip::NodeId nodeId);

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
