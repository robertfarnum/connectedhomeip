
#include "RpcCommands.h"

#include <lib/support/logging/CHIPLogging.h>

#include "RpcClient.h"
#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
// #include "GrpcServer.h"
#include "control_server/SocketServer.h"
#endif /* CONFIG_ENABLE_GRPC */

#define MAX_MESSAGE_LEN 65

#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
CHIP_ERROR RpcStartGrpcServerCommand::Run()
{
    // return StartGrpcServer();
    return SocketServer().start();
}
#endif /* CONFIG_ENABLE_GRPC */

CHIP_ERROR RpcAdminAddrCommand::Run()
{
    RpcSetServerAddress(ipaddr.data());
    return CHIP_NO_ERROR;
}

CHIP_ERROR RpcStatusCommand::Run()
{
    int rpcStatus = RpcGetStatus();

    switch (rpcStatus)
    {
    case RPC_DISCONNECTED:
        ChipLogProgress(NotSpecified, "jf-admin NOT connected.");
        break;
    case RPC_CONNECTING:
        ChipLogProgress(NotSpecified, "Still searching for jf-admin.");
        break;
    case RPC_CONNECTED:
        ChipLogProgress(NotSpecified, "jf-admin connected.");
        break;
    default:
        ChipLogError(NotSpecified, "ERROR: Unknown RPC status (code=%d)!", rpcStatus);
        break;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR RpcOpenCommissioningWindowCommand::Run()
{
    return RpcOpenCommissioningWindow(window_timeout);
}
