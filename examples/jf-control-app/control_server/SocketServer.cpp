#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <pw_thread/test_thread_context.h>
#include <pw_thread/thread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ControlServer.h"
#include "SocketServer.h"

using namespace pw;

SocketServer::SocketServer() {}

ssize_t SocketServer::Send(Json::Value value)
{
    Json::StreamWriterBuilder writerBuilder;
    const std::string json = Json::writeString(writerBuilder, value);

    size_t messageLength = json.size();
    ChipLogProgress(NotSpecified, "messageLength = %ld", messageLength);
    ssize_t bytesSent = send(sockfd, &messageLength, sizeof(messageLength), 0);
    if (bytesSent < 0)
    {
        ChipLogError(NotSpecified, "Error sending message length: %ld, error = %ld", messageLength, bytesSent);
        return bytesSent;
    }

    ChipLogProgress(NotSpecified, "header send bytes = %ld", bytesSent);

    bytesSent = send(sockfd, json.c_str(), json.size(), 0);
    if (bytesSent < 0)
    {
        ChipLogError(NotSpecified, "Error sending message: %ld", bytesSent);
        return bytesSent;
    }

    ChipLogProgress(NotSpecified, "data send bytes = %ld", bytesSent);

    ChipLogProgress(NotSpecified, "Sent message = %s, len = %ld", json.c_str(), json.size());

    return 0;
}

void SocketServer::handleConnection()
{
    ControlServer controlServer;

    bool firstAttempt = true;

    while (true)
    {
        // Receive the message length
        size_t messageLength;
        ssize_t bytesReceived = recv(sockfd, &messageLength, sizeof(messageLength), 0);
        if (bytesReceived <= 0)
        {
            // Handle error or disconnection
            ChipLogError(NotSpecified, "Recv length Error: %ld", bytesReceived);
            if (firstAttempt)
            {
                firstAttempt = false;
                continue;
            }

            break;
        }

        // ChipLogProgress(NotSpecified, "Received message len = %d", messageLength);

        // Receive the protobuf message
        std::string buffer(messageLength, '\0');
        size_t totalBytesReceived = 0;
        while (totalBytesReceived < messageLength)
        {
            ssize_t bytes = recv(sockfd, &buffer[totalBytesReceived], messageLength - totalBytesReceived, 0);
            // ChipLogProgress(NotSpecified, "Received bytes = %ld", bytes);
            if (bytes <= 0)
            {
                // Handle error or disconnection
                ChipLogError(NotSpecified, "Recv message Error: %ld", bytes);
                break;
            }
            totalBytesReceived += bytes;
        }

        Json::Value root;
        JSONCPP_STRING err;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        const auto rawJsonLength = static_cast<int>(buffer.length());

        if (!reader->parse(buffer.c_str(), buffer.c_str() + rawJsonLength, &root, &err))
        {
            ChipLogError(NotSpecified, "Json Error parsing buffer");
            break;
        }

        const std::string method = root["method"].asString();
        const Json::Value data   = root["data"];

        if (method != "GetDevices")
        {
            ChipLogProgress(NotSpecified, "Received message = %s, len = %ld", buffer.c_str(), totalBytesReceived);
        }

        Json::Value value = controlServer.HandleMethod(method, data);
        if (Send(value) < 0)
        {
            break;
        }
    }

    ChipLogProgress(NotSpecified, "Client disconnected");

    close(sockfd);
}

void SocketServer::waitForConnection(void)
{
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        ChipLogError(NotSpecified, "ERROR: socket failed");
        return;
    }
    const int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        ChipLogError(NotSpecified, "ERROR: setsockop failed");
    }

    // Bind the socket to an address and port
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(socketServerPort);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        ChipLogError(NotSpecified, "ERROR: bind failed");
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        ChipLogError(NotSpecified, "ERROR: listen failed");
        return;
    }

    ChipLogProgress(NotSpecified, "Listening for client to connect");

    for (;;)
    {
        // Accept a connection
        if ((sockfd = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0)
        {
            ChipLogError(NotSpecified, "ERROR: accept failed");
            break;
        }
        ChipLogProgress(NotSpecified, "Client connected");

        /* Create a thread dedicated to the server */
        thread::stl::Options options;
        thread::Thread socketServerThread(options, [this]() { handleConnection(); });
        socketServerThread.detach();
    }

    close(server_fd);
    server_fd = -1;

    return;
}

CHIP_ERROR SocketServer::start(void)
{
    SocketServer socketServer;

    /* Create a thread dedicated to the GRPC server */
    thread::stl::Options options;
    thread::Thread socketServerThread(options, [&socketServer]() { socketServer.waitForConnection(); });
    socketServerThread.detach();

    return CHIP_NO_ERROR;
}
