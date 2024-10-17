
#pragma once

#include <commands/common/CHIPCommand.h>

#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
class RpcStartGrpcServerCommand : public Command
{
public:
    RpcStartGrpcServerCommand() : Command("start-grpc", "Bring up the GRPC server.") { }

    CHIP_ERROR Run();
};
#endif /* CONFIG_ENABLE_GRPC */

class RpcAdminAddrCommand : public Command
{
public:
    RpcAdminAddrCommand() : Command("admin-addr", "Configure jf-admin IP address.")
    {
        AddArgument("ip-addr", &ipaddr,
                     "The IP address of the jf-admin.");
    }

    CHIP_ERROR Run();

private:
    chip::CharSpan ipaddr;
};

class RpcStatusCommand : public Command
{
public:
    RpcStatusCommand() : Command("status", "RPC connection status.") { }

    CHIP_ERROR Run();
};

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
