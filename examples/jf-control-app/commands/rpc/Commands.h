
#pragma once

#include <commands/common/Commands.h>
#include <commands/rpc/RpcCommands.h>
#include <commands/rpc/RpcConnect.h>

void registerCommandsRpc(Commands & commands)
{
    const char * clusterName = "RPC";

    commands_list clusterCommands = {
        make_unique<RpcConnectCommand>(),
        make_unique<RpcAdminAddrCommand>(),
#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
        make_unique<RpcStartGrpcServerCommand>(),
#endif
        make_unique<RpcStatusCommand>(),
        make_unique<RpcSendCommand>(),
        make_unique<RpcOpenCommissioningWindowCommand>()
    };

    commands.RegisterCommandSet(clusterName, clusterCommands, "Commands for RPC communication.");
}
