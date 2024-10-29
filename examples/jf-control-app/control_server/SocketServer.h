#include <lib/core/CHIPError.h>

class SocketServer
{
public:
    SocketServer();

    CHIP_ERROR start();

private:
    void handleConnection();
    void waitForConnection();

    const static int socketServerPort = 8112;
    int sockfd                        = -1;
};
