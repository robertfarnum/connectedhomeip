
#pragma once

#include <commands/common/CHIPCommand.h>

#define DEFAULT_ITERATIONS "1000"
#define DEFAULT_DISCRIMINATOR "3840"

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
                     "The new IP address of the jf-admin.");
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
        AddArgument("mode", 0, 1, &mode, "0-BCM or 1-ECM.");
        AddArgument("window_timeout", 0, 65535, &window_timeout, "Window timeout.");
        AddArgument("iterations", 1000, 100000, &iterations, "Iterations. Default: " DEFAULT_ITERATIONS);
        AddArgument("discriminator", 0, 4095, &discriminator, "Discriminator. Default: " DEFAULT_DISCRIMINATOR);
    }
    CHIP_ERROR Run();

private:
    uint16_t mode;
    uint16_t window_timeout;
    chip::Optional<uint32_t> iterations;
    chip::Optional<uint16_t> discriminator;
};
