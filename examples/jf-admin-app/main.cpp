
#include "RpcServer.h"
#include "pw_rpc_system_server/rpc_server.h"
#include "pw_rpc_system_server/socket.h"
#include <lib/support/logging/CHIPLogging.h>

#define RPC_SERVER_PORT         8111

int main(int argc, char * argv[])
{
    test::TestImpl test_service;

    ChipLogProgress(NotSpecified, "Initializing jf-admin-app RPC server...");
    pw::rpc::system_server::set_socket_port(RPC_SERVER_PORT);
    pw::rpc::system_server::Init();
    pw::rpc::system_server::Server().RegisterService(test_service);

    /* Start RPC server. Never returns... Will probably have to do something
     * about that... */
    pw::rpc::system_server::Start();

    return 0;
}
