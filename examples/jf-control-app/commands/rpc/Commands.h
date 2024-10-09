
#pragma once

#include <commands/common/Commands.h>
#include <commands/rpc/RpcCommands.h>

void registerCommandsRpc(Commands & commands)
{
    const char * clusterName = "RPC";

    commands_list clusterCommands = {
        make_unique<RpcSendCommand>()
    };

    commands.RegisterCommandSet(clusterName, clusterCommands, "Commands for RPC communication.");
}
