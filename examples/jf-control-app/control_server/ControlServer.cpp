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
    DeviceDatastoreCacheInstance().AddListener(this);

    std::vector<DeviceEntry> deviceEntries = DeviceDatastoreCacheInstance().GetDeviceDatastoreCache();

    for (auto & deviceEntry : deviceEntries)
    {
        deviceEntry.AddListener(this);
        addDevice(deviceEntry);
    }

    // Json::Value adminDevice;
    // adminDevice["nodeId"]          = chip::NodeId(1);
    // adminDevice["friendlyName"]    = "NXP Administrator";
    // adminDevice["reachable"]       = true;
    // adminDevice["vendorName"]      = "NXP";
    // adminDevice["productName"]     = "RW612";
    // adminDevice["hardwareVersion"] = "1.0";
    // adminDevice["softwareVersion"] = "1.0";
    // adminDevice["type"]            = 1;
    // devices.append(adminDevice);
}

ControlServer::~ControlServer()
{
    std::vector<DeviceEntry> deviceEntries = DeviceDatastoreCacheInstance().GetDeviceDatastoreCache();

    for (auto & deviceEntry : deviceEntries)
    {
        deviceEntry.RemoveListener(this);
    }

    DeviceDatastoreCacheInstance().RemoveListener(this);
}

Json::Value * ControlServer::findDevice(chip::NodeId nodeId)
{
    Json::Value * device = NULL;

    ChipLogProgress(NotSpecified, "findDevice(%lu), size = %d", nodeId, devices.size());

    for (unsigned int index = 0; index < devices.size(); index++)
    {
        chip::NodeId deviceNodeId = devices[index]["nodeId"].asUInt64();
        if (deviceNodeId == nodeId)
        {
            device = &devices[index];
        }
    }

    return device;
}

void ControlServer::updateDevice(Json::Value & device, DeviceEntry & deviceEntry)
{

    chip::NodeId nodeId = deviceEntry.GetNodeId();

    ChipLogProgress(NotSpecified, "updateDevice(%lu) called", nodeId);

    ChipLogProgress(JointFabric, "NodeID: %lu, friendlyName: %s", deviceEntry.GetNodeId(), deviceEntry.GetFriendlyName().data());
    ChipLogProgress(JointFabric, "VendorName: %s, ProductName: %s", deviceEntry.GetVendorName().data(),
                    deviceEntry.GetProductName().data());
    ChipLogProgress(JointFabric, "Reachable: %d, HW-Version: %s, SW-Version: %s, On: %d", deviceEntry.GetReachable(),
                    deviceEntry.GetHardwareVersionString().data(), deviceEntry.GetSoftwareVersionString().data(),
                    deviceEntry.GetOn());
    ChipLogProgress(JointFabric, "Type:%d, OnOffSubscriptionEstablished: %d", deviceEntry.GetType(),
                    deviceEntry.GetOnOffSubscriptionEstablished());

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
}

void ControlServer::addDevice(DeviceEntry & deviceEntry)
{
    chip::NodeId nodeId = deviceEntry.GetNodeId();

    ChipLogProgress(NotSpecified, "addDevice(%lu) called", nodeId);

    if (findDevice(nodeId) != NULL)
    {
        ChipLogProgress(NotSpecified, "addDevice(); device already exists = %lu", nodeId);
        return;
    }

    deviceEntry.AddListener(this);

    Json::Value device;
    updateDevice(device, deviceEntry);
    devices.append(device);
}

void ControlServer::removeDevice(DeviceEntry & deviceEntry)
{
    chip::NodeId nodeId = deviceEntry.GetNodeId();

    ChipLogProgress(NotSpecified, "removeDevice(%lu) called", nodeId);

    deviceEntry.RemoveListener(this);

    for (Json::ArrayIndex index = 0; index < devices.size(); index++)
    {
        Json::Value device        = devices[index];
        chip::NodeId deviceNodeId = device["nodeId"].asUInt64();
        ChipLogProgress(NotSpecified, "%lu == %lu", deviceNodeId, nodeId);

        if (deviceNodeId == nodeId)
        {
            Json::Value removed;
            devices.removeIndex(index, &removed);
            break;
        }
    }
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

    result["devices"]   = devices;
    result["errorCode"] = 0;

    return result;
}

void ControlServer::updateFriendlyName(Json::Value & device, std::string friendlyName)
{
    if (device["friendlyName"].asString() != friendlyName)
    {
        const chip::NodeId nodeId = std::stoull(device["nodeId"].asString());
        device["friendlyName"]    = friendlyName;

        StringBuilder<kMaxCommandSize> commandBuilder;
        commandBuilder.Add("basicinformation write node-label ");
        commandBuilder.AddFormat("'%s' %lu %d ", friendlyName.c_str(), nodeId, 0);
        PushCommand(commandBuilder.c_str());
    }
}

void ControlServer::updateOnOff(Json::Value & device, bool on)
{
    const int deviceType = device["type"].asInt();

    ChipLogProgress(NotSpecified, "deviceType = %d", deviceType);

    if (deviceType == 0 && device["on"] != on)
    {
        device["on"] = on;

        const chip::NodeId nodeId = std::stoull(device["nodeId"].asString());

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

    Json::Value * device = findDevice(nodeId);
    if (device == NULL)
    {
        result["errorCode"] = 0;

        return result;
    }

    updateFriendlyName(*device, friendlyName);
    updateOnOff(*device, data["on"].asBool());

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
