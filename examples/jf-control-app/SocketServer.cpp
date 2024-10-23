#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "pw_thread/thread.h"
#include <pw_thread/test_thread_context.h>
#include "SocketServer.h"

using namespace pw;

static uint16_t socketServerPort = 8112;
static int new_socket = -1;

static void handleClientConnection() {
    char buffer[2048] = {0};
    int size = sizeof(buffer);

    for (;new_socket != -1;) {
        // Read data from client
        if(read(new_socket, buffer, size)) {
            break;
        }
        std::cout << "Client: " << buffer << std::endl;
    }
}

static void RunSocketServer(void) {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "socket failed" << std::endl;
        return;
    }

    // Bind the socket to an address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(socketServerPort);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "listen failed" << std::endl;
        return;
    }

    for (;;) {
        // Accept a connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "accept failed" << std::endl;
            break;
        }

        handleClientConnection();
    }

    // Close the socket
    close(new_socket);
    new_socket = -1;

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
    if (new_socket == -1) {
        return -1;
    }

    // Send data to client
    if(send(new_socket, message, size, 0) == 0) {
        close(new_socket);
        new_socket = -1;
    }

    return -1;
}
