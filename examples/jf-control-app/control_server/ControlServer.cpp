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

ControlServer::ControlServer() {}

ControlServer::~ControlServer() {}

Json::Value ControlServer::getDevice(DeviceEntry & deviceEntry)
{
    Json::Value device;

    chip::NodeId nodeId = deviceEntry.GetNodeId();
    /*
        ChipLogProgress(NotSpecified, "getDevice(%lu) called", nodeId);

        ChipLogProgress(JointFabric, "NodeID: %lu, friendlyName: %s", deviceEntry.GetNodeId(),
       deviceEntry.GetFriendlyName().data()); ChipLogProgress(JointFabric, "VendorName: %s, ProductName: %s",
       deviceEntry.GetVendorName().data(), deviceEntry.GetProductName().data()); ChipLogProgress(JointFabric, "Reachable: %d,
       HW-Version: %s, SW-Version: %s, On: %d", deviceEntry.GetReachable(), deviceEntry.GetHardwareVersionString().data(),
       deviceEntry.GetSoftwareVersionString().data(), deviceEntry.GetOn()); ChipLogProgress(JointFabric, "Type:%d,
       OnOffSubscriptionEstablished: %d", deviceEntry.GetType(), deviceEntry.GetOnOffSubscriptionEstablished());
    */

    std::string friendlyName = std::string(deviceEntry.GetFriendlyName().data());
    uint8_t type             = deviceEntry.GetType();

    if (friendlyName == "")
    {
        switch (type)
        {
        case 0:
            friendlyName = std::string("Light Bulb ");
            break;
        case 1:
            friendlyName = std::string("Joint Fabric Administrator ");
            break;
        case 3:
            friendlyName = std::string("Switch ");
            break;
        }
        friendlyName += std::to_string(nodeId);
    }

    device["nodeId"]          = deviceEntry.GetNodeId();
    device["friendlyName"]    = friendlyName;
    device["reachable"]       = deviceEntry.GetReachable();
    device["on"]              = deviceEntry.GetOn();
    device["vendorName"]      = std::string(deviceEntry.GetVendorName().data());
    device["productName"]     = std::string(deviceEntry.GetProductName().data());
    device["hardwareVersion"] = std::string(deviceEntry.GetHardwareVersionString().data());
    device["softwareVersion"] = std::string(deviceEntry.GetSoftwareVersionString().data());
    device["type"]            = deviceEntry.GetType();

    return device;
}

Json::Value ControlServer::handleOpenCommissioningWindow(Json::Value data)
{
    Json::Value result;

    ChipLogProgress(NotSpecified, "handleOpenCommissioningWindow called");

    const chip::NodeId nodeId     = std::stoull(data["nodeId"].asString());
    const u_int32_t option        = data["option"].asUInt();
    const u_int32_t windowTimeout = data["windowTimeout"].asUInt();
    const u_int32_t iteration     = data["iteration"].asUInt();
    const u_int32_t discriminator = data["discriminator"].asUInt();

    ChipLogProgress(NotSpecified,
                    "handleOpenCommissioningWindow(nodeId=%lu, option=%d, window_timeout=%d, iteration=%d, discriminator=%d ",
                    nodeId, option, windowTimeout, iteration, discriminator);

    StringBuilder<kMaxCommandSize> commandBuilder;
    commandBuilder.Add("pairing open-commissioning-window ");
    commandBuilder.AddFormat("%lu %d %d %d %d", nodeId, option, windowTimeout, iteration, discriminator);
    PushCommand(commandBuilder.c_str());

    result["errorCode"] = 0;

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

    const std::string duration           = data["duration"].asString();
    const std::string ssid               = data["ssid"].asString();
    const std::string password           = data["password"].asString();
    const std::string setupPinCode       = data["setupPinCode"].asString();
    const std::string discriminator      = data["discriminator"].asString();
    const std::string operationalDataset = data["operationalDataset"].asString();
    const bool useBle                    = data["useBle"].asBool();
    const bool useBleWiFi                = data["useBleWiFi"].asBool();
    const bool useBleThread              = data["useBleThread"].asBool();
    const bool useOnNetwork              = data["useOnNetwork"].asBool();

    ChipLogProgress(
        NotSpecified,
        "handleCommissionDevice(setup_pin_code=\"%s\", discriminator=\"%s\", useBleWiFi=%d, useBleThread=%d, useOnNetwork=%d, "
        "duration=\"%s\"), ssid=\"%s\"",
        setupPinCode.c_str(), discriminator.c_str(), useBleWiFi, useBleThread, useOnNetwork, duration.c_str(), ssid.c_str());

    if (useBle || useBleWiFi)
    {
        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("pairing ble-wifi ");
        commandBuilder.AddFormat("%lu %s %s %s %s --bypass-attestation-verifier 1", nextNodeId, ssid.c_str(), password.c_str(),
                                 setupPinCode.c_str(), discriminator.c_str());
        PushCommand(commandBuilder.c_str());
    }
    if (useBleThread)
    {
        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("pairing ble-thread ");
        commandBuilder.AddFormat("%lu %s %s %s --bypass-attestation-verifier 1", nextNodeId, operationalDataset.c_str(),
                                 setupPinCode.c_str(), discriminator.c_str());
        PushCommand(commandBuilder.c_str());
    }
    else if (useOnNetwork)
    {
        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("pairing onnetwork ");
        commandBuilder.AddFormat("%lu %s --bypass-attestation-verifier 1", nextNodeId, setupPinCode.c_str());
        PushCommand(commandBuilder.c_str());
    }
    else
    {
        ChipLogError(NotSpecified, "Neither ble or onnetwork was selected");
        result["errorCode"] = -1;
        return result;
    }

    nextNodeId++;

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::handleGetDevices(Json::Value data)
{
    Json::Value result;
    std::vector<DeviceEntry> deviceEntries = DeviceDatastoreCacheInstance().GetDeviceDatastoreCache();
    Json::Value devices                    = Json::Value(Json::arrayValue);

    for (auto & deviceEntry : deviceEntries)
    {
        Json::Value device = getDevice(deviceEntry);
        devices.append(device);
    }

    result["devices"]   = devices;
    result["errorCode"] = 0;

    return result;
}

void ControlServer::updateFriendlyName(DeviceEntry & deviceEntry, std::string friendlyName)
{
    const chip::NodeId nodeId = deviceEntry.GetNodeId();

    std::string oldFriendlyName = std::string(deviceEntry.GetFriendlyName().data());
    if (oldFriendlyName != friendlyName)
    {
        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("basicinformation write node-label ");
        commandBuilder.AddFormat("'%s' %lu %d ", friendlyName.c_str(), nodeId, 0);
        PushCommand(commandBuilder.c_str());
    }
}

void ControlServer::updateOnOff(DeviceEntry & deviceEntry, bool on)
{
    const chip::NodeId nodeId = deviceEntry.GetNodeId();

    if (deviceEntry.GetType() != 1)
    {
        StringBuilder<kMaxCommandSize> commandBuilder;
        if (on)
        {
            commandBuilder.Add("onoff on ");
            commandBuilder.AddFormat("%lu %d ", nodeId, 1);
        }
        else
        {
            commandBuilder.Add("onoff off ");
            commandBuilder.AddFormat("%lu %d ", nodeId, 1);
        }
        PushCommand(commandBuilder.c_str());
    }
}

Json::Value ControlServer::handleControlDevice(Json::Value data)
{
    Json::Value result;
    const chip::NodeId nodeId      = std::stoull(data["nodeId"].asString());
    const std::string friendlyName = data.get("friendlyName", "").asString();

    ChipLogProgress(NotSpecified, "handleControlDevice(nodeId=%lu, friendlyName=\"%s\", on=%d)", nodeId, friendlyName.c_str(),
                    data["on"].asBool());

    std::vector<DeviceEntry> deviceEntries = DeviceDatastoreCacheInstance().GetDeviceDatastoreCache();
    for (auto & deviceEntry : deviceEntries)
    {
        if (deviceEntry.GetNodeId() == nodeId)
        {
            updateFriendlyName(deviceEntry, friendlyName);
            updateOnOff(deviceEntry, data["on"].asBool());
        }
    }

    result["errorCode"] = 0;

    return result;
}

Json::Value ControlServer::handleDeleteDevice(Json::Value data)
{
    Json::Value result;

    const chip::NodeId nodeId = std::stoull(data["nodeId"].asString());

    ChipLogProgress(NotSpecified, "handleDeleteDevice(nodeId=%lu)", nodeId);

    StringBuilder<kMaxCommandSize> commandBuilder;
    commandBuilder.Add("pairing unpair ");
    commandBuilder.AddFormat("%lu ", nodeId);
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
