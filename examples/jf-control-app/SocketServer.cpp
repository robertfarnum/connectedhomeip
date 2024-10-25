#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <json/json.h>
#include <pw_thread/thread.h>
#include <pw_thread/test_thread_context.h>

#include <lib/support/logging/CHIPLogging.h>
#include <rpc_services/JointFabric.h>
#include <commands/common/CHIPCommand.h>
#include <commands/common/CHIPCommand.h>
#include <commands/interactive/InteractiveCommands.h>
#include <lib/support/StringBuilder.h>

#include "JFAdmin.h"
#include "SocketServer.h"

using namespace pw;
using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

static uint16_t socketServerPort = 8112;
static int sockfd = -1;

static Json::Value handleOpenCommissioningWindow(Json::Value data) {
    Json::Value result;

    ChipLogProgress(NotSpecified, "handleOpenCommissioningWindow called");

    return result;
}

static Json::Value handleCommissionAdminDevice(Json::Value data) {
    Json::Value result;

    const std::string manualCode = data["manualCode"].asString();
    const std::string duration = data["duration"].asString();

    ChipLogProgress(NotSpecified, "handleCommissionAdminDevice(manual_code=\"%s\", duration=\"%s\")",
        manualCode.c_str(), duration.c_str());

    JointFabricAdmin::GetInstance().OnboardAdmin(manualCode.c_str());

    result["errorCode"] = 0;

    return result;
}

static Json::Value handleCommissionDevice(Json::Value data) {
    Json::Value result;

    const std::string manualCode = data["manualCode"].asString();
    const std::string duration = data["duration"].asString();

    ChipLogProgress(NotSpecified, "handleCommissionDevice(manual_code=\"%s\", duration=\"%s\")",
        manualCode.c_str(), duration.c_str());

    // TODO: Implemenent

    result["errorCode"] = 0;

    return result;
}

static bool on = false;
static std::string friendlyName = "Test 1";
static std::string nodeId = "10";

static Json::Value handleGetDevices(Json::Value data) {
    Json::Value result;

    //ChipLogProgress(NotSpecified, "handleGetDevices()");

    // TODO: Replace with actual devices
    Json::Value devices(Json::arrayValue);

    // TODO: Loop start here
    Json::Value device;
    device["nodeId"] = nodeId;
    device["friendlyName"] = friendlyName;
    device["connected"] = true;
    device["on"] = on;
    device["manufacturer"] = "tapo";
    device["model"] = "tapo";
    device["hardwareVersion"] = "1.0";
    device["firmwareVersion"] = "1.0";
    devices.append(device);
    // TODO: Loop end here

    result["devices"] =  devices;

    result["errorCode"] = 0;

    return result;
}

static Json::Value handleControlDevice(Json::Value data) {
    Json::Value result;

    const std::string nodeId = data.get("nodeId", "UTF-8").asString();
    friendlyName = data.get("friendlyName", "UTF-8").asString();
    const std::string onStr = data["on"].asString();
    on = (onStr == ("true"));

    ChipLogProgress(NotSpecified, "handleControlDevice(nodeId=\"%s\", friendlyName=\"%s\", on=\"%d\")",
        nodeId.c_str(), friendlyName.c_str(), on);

    StringBuilder<kMaxCommandSize> commandBuilder;
    if (on) {
        commandBuilder.Add("onoff on ");
        commandBuilder.AddFormat("%s %d ", nodeId.c_str(), 1);
    } else {
        commandBuilder.Add("onoff off ");
        commandBuilder.AddFormat("%s %d ", nodeId.c_str(), 1);
    }
    PushCommand(commandBuilder.c_str());

    if (friendlyName != "") {
        StringBuilder<kMaxCommandSize> commandBuilder;

        commandBuilder.Add("basicinformation write node-label ");
        commandBuilder.AddFormat("'%s' %s %d ", friendlyName.c_str(), nodeId.c_str(), 0);

        PushCommand(commandBuilder.c_str());
    }

    result["errorCode"] = 0;

    return result;
}

std::map<std::string, Json::Value (*)(Json::Value)> methodHandlers = {
    {"OpenCommissioningWindow", handleOpenCommissioningWindow},
    {"CommissionDevice", handleCommissionDevice},
    {"CommissionAdminDevice", handleCommissionAdminDevice},
    {"GetDevices", handleGetDevices},
    {"ControlDevice", handleControlDevice},
};

static Json::Value handleMethod(std::string method, Json::Value data) {
    Json::Value result;

    //ChipLogProgress(NotSpecified, "method = %s", method.c_str());

    auto it = methodHandlers.find(method);
    if (it != methodHandlers.end()) {
        result = (it->second)(data);
    } else {
        std::cout << "Unknown method!" << std::endl;
    }

    result["method"] = method;

    return result;
}

static void handleClientConnection() {
    while (true) {
        // Receive the message length
        int32_t messageLength;
        int bytesReceived = recv(sockfd, &messageLength, sizeof(messageLength), 0);
        if (bytesReceived <= 0) {
            // Handle error or disconnection
            break;
        }

        //ChipLogProgress(NotSpecified, "Received message len = %d", messageLength);

        // Receive the protobuf message
        std::string buffer(messageLength, '\0');
        int totalBytesReceived = 0;
        while (totalBytesReceived < messageLength) {
            int bytes = recv(sockfd, &buffer[totalBytesReceived], messageLength - totalBytesReceived, 0);
            //ChipLogProgress(NotSpecified, "Received bytes = %d", bytes);
            if (bytes <= 0) {
                // Handle error or disconnection
                break;
            }
            totalBytesReceived += bytes;
        }

        //ChipLogProgress(NotSpecified, "Received message = %s, len = %d", buffer.c_str(), totalBytesReceived);

        Json::Value root;
        JSONCPP_STRING err;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        const auto rawJsonLength = static_cast<int>(buffer.length());

        if (!reader->parse(buffer.c_str(), buffer.c_str() + rawJsonLength, &root, &err)) {
            ChipLogError(NotSpecified, "Error");
            break;
        }

        const std::string method = root["method"].asString();
        const Json::Value data = root["data"];

        Json::Value result = handleMethod(method, data);
        Json::StreamWriterBuilder writerBuilder;
        const std::string json = Json::writeString(writerBuilder, result);

        messageLength = json.size();
        int bytesSent = send(sockfd, &messageLength, sizeof(messageLength), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending message length" << std::endl;
            break;
        }

        bytesSent = send(sockfd, json.c_str(), json.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending message data" << std::endl;
            break;
        }
    }
}

static void RunSocketServer(void) {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        ChipLogError(NotSpecified, "ERROR: socket failed");
        return;
    }

    // Bind the socket to an address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(socketServerPort);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ChipLogError(NotSpecified, "ERROR: bind failed");
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        ChipLogError(NotSpecified, "ERROR: listen failed");
        return;
    }

    ChipLogProgress(NotSpecified, "Listening for client to connect");

    for (;;) {
        // Accept a connection
        if ((sockfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            ChipLogError(NotSpecified, "ERROR: accept failed");
            break;
        }
        ChipLogProgress(NotSpecified, "Client connected");

        handleClientConnection();
    }

    // Close the socket
    close(sockfd);
    sockfd = -1;

    close(server_fd);
    server_fd = -1;

    return;
}

CHIP_ERROR StartSocketServer(void)
{
    /* Create a thread dedicated to the GRPC server */
    thread::stl::Options options;
    thread::Thread socketServerThread(options, RunSocketServer);
    socketServerThread.detach();

    return CHIP_NO_ERROR;
}

int SendMessage(char *message, int size) {
    if (sockfd == -1) {
        return -1;
    }

    // Send data to client
    if(send(sockfd, message, size, 0) == 0) {
        close(sockfd);
        sockfd = -1;
    }

    return -1;
}
