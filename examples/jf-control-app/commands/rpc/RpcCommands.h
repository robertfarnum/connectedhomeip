
#pragma once

#include <commands/common/CHIPCommand.h>

class RpcSendCommand : public Command
{
public:
    RpcSendCommand() : Command("send", "Send a message to the jf-admin. The maximum message length is 64 chars.")
    {
        AddArgument("message", &message,
                     "The message to relay to the jf-admin.");
    }

    CHIP_ERROR Run() { return RunCommand(message); }

private:
    chip::CharSpan message;

    CHIP_ERROR RunCommand(chip::CharSpan & msg);
};

#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
class RpcStartGrpcServerCommand : public Command
{
public:
    RpcStartGrpcServerCommand() : Command("start-grpc", "Bring up the GRPC server.") { }

    CHIP_ERROR Run();
};
#endif /* CONFIG_ENABLE_GRPC */

class RpcOpenCommissioningWindowCommand : public Command
{
public:
    RpcOpenCommissioningWindowCommand() : Command("open-commissioning-window", "Open commissioning window.")
    {
        AddArgument("window_timeout", 0, 65535, &window_timeout,
                     "Window timeout.");
    }
    CHIP_ERROR Run();

private:
    uint16_t window_timeout;
};
