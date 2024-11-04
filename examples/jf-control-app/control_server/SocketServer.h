#include <lib/core/CHIPError.h>

class SocketServer
{
public:
    SocketServer();
    ssize_t Send(Json::Value value);

    CHIP_ERROR start();

private:
    friend SocketServer & SockSrv();

    void handleConnection();
    void waitForConnection();

    const static int socketServerPort = 8112;
    int sockfd                        = -1;
    static SocketServer sInstance;
};

inline SocketServer & SockSrv()
{
    return SocketServer::sInstance;
}
