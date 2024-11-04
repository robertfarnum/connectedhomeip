#include <json/json.h>
#include <lib/core/CHIPError.h>
#include <lib/support/logging/CHIPLogging.h>

#include "device_manager/DeviceDatastoreCache.h"

class ControlServer
{
public:
    ControlServer();
    ~ControlServer();

    Json::Value HandleMethod(std::string method, Json::Value data);

private:
    void updateOnOff(DeviceEntry & deviceEntry, bool on);
    void updateFriendlyName(DeviceEntry & deviceEntry, std::string friendlyName);

    // Method handlers
    Json::Value handleDeleteDevice(Json::Value data);
    Json::Value handleControlDevice(Json::Value data);
    Json::Value handleGetDevices(Json::Value data);
    Json::Value handleCommissionDevice(Json::Value data);
    Json::Value handleCommissionAdminDevice(Json::Value data);
    Json::Value handleOpenCommissioningWindow(Json::Value data);
    Json::Value getDevice(DeviceEntry & deviceEntry);

    chip::NodeId nextNodeId = 20;

    std::map<std::string, Json::Value (ControlServer::*)(Json::Value)> methodHandlers = {
        { "OpenCommissioningWindow", &ControlServer::handleOpenCommissioningWindow },
        { "CommissionDevice", &ControlServer::handleCommissionDevice },
        { "CommissionAdminDevice", &ControlServer::handleCommissionAdminDevice },
        { "GetDevices", &ControlServer::handleGetDevices },
        { "ControlDevice", &ControlServer::handleControlDevice },
        { "DeleteDevice", &ControlServer::handleDeleteDevice }
    };
};
