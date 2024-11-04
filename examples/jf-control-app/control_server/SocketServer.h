#include <lib/core/CHIPError.h>

class SocketServer
{
public:
    static SocketServer sInstance;

    SocketServer();
    ssize_t Send(Json::Value value);

    CHIP_ERROR start();

private:
    void handleConnection();
    void waitForConnection();

    const static int socketServerPort = 8112;
    int sockfd                        = -1;
};
