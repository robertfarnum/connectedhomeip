#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "pw_thread/thread.h"
#include <pw_thread/test_thread_context.h>
#include <lib/support/logging/CHIPLogging.h>
#include "json/json.h"
#include "rpc_services/JointFabric.h"
#include "JFAdmin.h"
#include "SocketServer.h"

using namespace pw;

static uint16_t socketServerPort = 8112;
static int sockfd = -1;

static Json::Value handleOpenCommissioningWindow(Json::Value params) {
    Json::Value result;

    ChipLogProgress(NotSpecified, "handleOpenCommissioningWindow called");

    return result;
}

static Json::Value handleCommissionDevice(Json::Value params) {
    Json::Value result;

    const std::string manualCode = params["manualCode"].asString();
    const std::string duration = params["duration"].asString();

    ChipLogProgress(NotSpecified, "handleCommissionDevice(manual_code=\"%s\", duration=\"%s\")",
        manualCode.c_str(), duration.c_str());

    JointFabricAdmin::GetInstance().OnboardAdmin(manualCode.c_str());

    result["errorCode"] = 0;

    return result;
}

static Json::Value handleGetDevices(Json::Value params) {
    Json::Value result;

    ChipLogProgress(NotSpecified, "handleGetDevices()");

    // TODO: Replace with actual devices
    Json::Value devices(Json::arrayValue);
    Json::Value device;
    device["nodeId"] = 10;
    device["displayName"] = "Test 1";
    device["on"] = true;
    devices.append(device);
    result["devices"] =  devices;

    result["errorCode"] = 0;

    return result;
}

static Json::Value handleControlDevice(Json::Value device) {
    Json::Value result;

    const std::string displayName = device["displayName"].asString();
    const bool on = device["on"].asBool();
    const uint nodeId = device["nodeId"].asUInt();

    ChipLogProgress(NotSpecified, "handleControlDevice(nodeId=\"%ul\", displayName=\"%s\", on=\"%d\")",
        nodeId, displayName, on);

    result["errorCode"] = 0;

    return result;
}

std::map<std::string, Json::Value (*)(Json::Value)> methodHandlers = {
    {"OpenCommissioningWindow", handleOpenCommissioningWindow},
    {"CommissionDevice", handleCommissionDevice},
    {"GetDevices", handleGetDevices},
    {"ControlDevice", handleControlDevice},
};

static Json::Value handleMethod(std::string method, Json::Value params) {
    Json::Value result;

    ChipLogProgress(NotSpecified, "method = %s", method.c_str());

    auto it = methodHandlers.find(method);
    if (it != methodHandlers.end()) {
        result = (it->second)(params);
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

        ChipLogProgress(NotSpecified, "Received message len = %d", messageLength);

        // Receive the protobuf message
        std::string buffer(messageLength, '\0');
        int totalBytesReceived = 0;
        while (totalBytesReceived < messageLength) {
            int bytes = recv(sockfd, &buffer[totalBytesReceived], messageLength - totalBytesReceived, 0);
            ChipLogProgress(NotSpecified, "Received bytes = %d", bytes);
            if (bytes <= 0) {
                // Handle error or disconnection
                break;
            }
            totalBytesReceived += bytes;
        }

        ChipLogProgress(NotSpecified, "Received message = %s, len = %d", buffer.c_str(), totalBytesReceived);

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
        const Json::Value params = root["params"];

        Json::Value result = handleMethod(method, params);
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
