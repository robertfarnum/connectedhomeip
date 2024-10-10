
#pragma once

#include <commands/common/CHIPCommand.h>

class RpcConnectCommand : public Command
{
public:
    RpcConnectCommand() : Command("connect", "Connect to the jf-admin-app. TBD: add IP/port as params.") {}

    CHIP_ERROR Run() { return RunCommand(); }

private:
    CHIP_ERROR RunCommand();
};
