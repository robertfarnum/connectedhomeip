#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <pw_thread/test_thread_context.h>
#include <pw_thread/thread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <commands/common/CHIPCommand.h>
#include <commands/interactive/InteractiveCommands.h>
#include <lib/support/StringBuilder.h>
#include <rpc_services/JointFabric.h>

#include "ControlServer.h"
#include "JFAdmin.h"

using namespace pw;
using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

ControlServer::ControlServer()
{
    ChipLogProgress(NotSpecified, "ControlServer() called");

    DeviceDatastoreCacheInstance().AddListener(this);

    std::vector<DeviceEntry> deviceEntries = DeviceDatastoreCacheInstance().GetDevices();

    for (auto & deviceEntry : deviceEntries)
    {
        deviceEntry.AddListener(this);
        addDevice(&deviceEntry);
    }

    // Json::Value lightDevice;
    // lightDevice["nodeId"]          = std::to_string(10);
    // lightDevice["friendlyName"]    = "Xfinity Light";
    // lightDevice["reachable"]       = true;
    // lightDevice["on"]              = false;
    // lightDevice["vendorName"]      = "tapo";
    // lightDevice["productName"]     = "tapo";
    // lightDevice["hardwareVersion"] = "1.0";
    // lightDevice["softwareVersion"] = "1.0";
    // lightDevice["type"]            = 0;
    // devices.append(lightDevice);

    // Json::Value adminDevice;
    // adminDevice["nodeId"]          = std::to_string(1);
    // adminDevice["friendlyName"]    = "NXP Administrator";
    // adminDevice["reachable"]       = true;
    // adminDevice["vendorName"]      = "NXP";
    // adminDevice["productName"]     = "RW612";
    // adminDevice["hardwareVersion"] = "1.0";
    // adminDevice["softwareVersion"] = "1.0";
    // adminDevice["type"]            = 1;
    // devices.append(adminDevice);
}

Json::Value * ControlServer::findDevice(chip::NodeId nodeId)
{
    Json::Value * device = NULL;

    for (unsigned int index = 0; index < devices.size(); index++)
    {
        if (devices[index]["nodeId"] == nodeId)
        {
            device = &devices[index];
        }
    }

    return device;
}

void ControlServer::updateDevice(Json::Value * device, DeviceEntry * deviceEntry)
{
    ChipLogProgress(NotSpecified, "updateDevice() called");

    if (device == NULL)
    {
        return;
    }

    (*device)["nodeId"]          = std::to_string(deviceEntry->GetNodeId());
    (*device)["friendlyName"]    = std::string(deviceEntry->GetFriendlyName().data());
    (*device)["reachable"]       = deviceEntry->GetReachable();
    (*device)["on"]              = deviceEntry->GetOn();
    (*device)["vendorName"]      = std::string(deviceEntry->GetVendorName().data());
    (*device)["productName"]     = std::string(deviceEntry->GetProductName().data());
    (*device)["hardwareVersion"] = std::string(deviceEntry->GetHardwareVersionString().data());
    (*device)["softwareVersion"] = std::string(deviceEntry->GetSoftwareVersionString().data());
    (*device)["type"]            = deviceEntry->GetType();
}

void ControlServer::addDevice(DeviceEntry * deviceEntry)
{
    ChipLogProgress(NotSpecified, "addDevice() called");

    if (deviceEntry == NULL)
    {
        return;
    }

    if (findDevice(deviceEntry->GetNodeId()) != NULL)
    {
        ChipLogProgress(NotSpecified, "addDevice(); device already exists");
        return;
    }

    deviceEntry->AddListener(this);

    Json::Value device;
    updateDevice(&device, deviceEntry);
    devices.append(device);
}

void ControlServer::removeDevice(chip::NodeId nodeId)
{
    ChipLogProgress(NotSpecified, "removeDevice(%lu) called", nodeId);
}

Json::Value ControlServer::handleOpenCommissioningWindow(Json::Value data)
{
    Json::Value result;

    ChipLogProgress(NotSpecified, "handleOpenCommissioningWindow called");

    return result;
}

Json::Value ControlServer::handleCommissionAdminDevice(Json::Value data)
{
    Json::Value result;

    const std::string setupPinCode = data["setupPinCode"].asString();
    const std::string duration     = data["duration"].asString();

    ChipLogProgress(NotSpecified, "handleCommissionAdminDevice(setup_pin_code=\"%s\", duration=\"%s\")", setupPinCode.c_str(),
                    duration.c_str());

    JointFabricAdmin::GetInstance().OnboardAdmin(setupPinCode.c_str());

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::handleCommissionDevice(Json::Value data)
{
    Json::Value result;

    const std::string duration      = data["duration"].asString();
    const std::string ssid          = data["ssid"].asString();
    const std::string password      = data["password"].asString();
    const std::string setupPinCode  = data["setupPinCode"].asString();
    const std::string discriminator = data["discriminator"].asString();

    ChipLogProgress(NotSpecified,
                    "handleCommissionDevice(setup_pin_code=\"%s\", discriminator=\"%s\", duration=\"%s\"), ssid=\"%s\"",
                    setupPinCode.c_str(), discriminator.c_str(), duration.c_str(), ssid.c_str());

    StringBuilder<kMaxCommandSize> commandBuilder;
    commandBuilder.Add("pairing ble-wifi ");
    commandBuilder.AddFormat("%lu %s %s %s %s --bypass-attestation-verifier 1", nextNodeId, ssid.c_str(), password.c_str(),
                             setupPinCode.c_str(), discriminator.c_str());
    PushCommand(commandBuilder.c_str());

    nextNodeId++;

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::handleGetDevices(Json::Value data)
{
    Json::Value result;

    result["devices"]   = devices;
    result["errorCode"] = 0;

    return result;
}

void ControlServer::updateFriendlyName(Json::Value * device, std::string friendlyName)
{
    if ((*device)["friendlyName"].asString() != friendlyName)
    {
        std::string nodeId        = (*device)["nodeId"].asString();
        (*device)["friendlyName"] = friendlyName;

        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("basicinformation write node-label ");
        commandBuilder.AddFormat("'%s' %s %d ", friendlyName.c_str(), nodeId.c_str(), 0);
        PushCommand(commandBuilder.c_str());
    }
}

void ControlServer::updateOnOff(Json::Value * device, bool on)
{
    const int deviceType = (*device)["type"].asInt();

    ChipLogProgress(NotSpecified, "deviceType = %d", deviceType);

    if (deviceType == 0 && (*device)["on"] != on)
    {
        (*device)["on"] = on;

        std::string nodeId = (*device)["nodeId"].asString();

        StringBuilder<kMaxCommandSize> commandBuilder;
        if (on)
        {
            commandBuilder.Add("onoff on ");
            commandBuilder.AddFormat("%s %d ", nodeId.c_str(), 1);
        }
        else
        {
            commandBuilder.Add("onoff off ");
            commandBuilder.AddFormat("%s %d ", nodeId.c_str(), 1);
        }
        PushCommand(commandBuilder.c_str());
    }
}

Json::Value ControlServer::handleControlDevice(Json::Value data)
{
    Json::Value result;
    const std::string nodeId       = data.get("nodeId", "UTF-8").asString();
    const std::string friendlyName = data.get("friendlyName", "UTF-8").asString();

    ChipLogProgress(NotSpecified, "handleControlDevice(nodeId=\"%s\", friendlyName=\"%s\", on=%d)", nodeId.c_str(),
                    friendlyName.c_str(), data["on"].asBool());

    Json::Value * device = NULL;

    for (unsigned int index = 0; index < devices.size(); index++)
    {
        if (devices[index]["nodeId"] == nodeId)
        {
            device = &devices[index];
        }
    }

    if (device == NULL)
    {
        result["errorCode"] = 0;

        return result;
    }

    updateFriendlyName(device, friendlyName);
    updateOnOff(device, data["on"].asBool());

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::handleDeleteDevice(Json::Value data)
{
    Json::Value result;

    const std::string nodeId = data.get("nodeId", "UTF-8").asString();

    ChipLogProgress(NotSpecified, "handleDeleteDevice(nodeId=\"%s\")", nodeId.c_str());

    StringBuilder<kMaxCommandSize> commandBuilder;
    commandBuilder.Add("pairing unpair ");
    commandBuilder.AddFormat("%s ", nodeId.c_str());
    PushCommand(commandBuilder.c_str());

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::HandleMethod(std::string method, Json::Value data)
{
    Json::Value result;

    if (method != "GetDevices")
    {
        ChipLogProgress(NotSpecified, "method = %s", method.c_str());
    }

    auto it = methodHandlers.find(method);
    if (it != methodHandlers.end())
    {
        result = (this->*(it->second))(data);
    }
    else
    {
        std::cout << "Unknown method!" << std::endl;
    }

    result["method"] = method;

    return result;
}
