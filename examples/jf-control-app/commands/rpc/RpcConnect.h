
#pragma once

#include <commands/common/CHIPCommand.h>

class RpcConnectCommand : public Command
{
public:
    RpcConnectCommand() : Command("connect", "Connect to the jf-admin.") {
        AddArgument("admin-addr", &ipaddr,
            "The IP address of the jf-admin. When omitted the app will look for"
            " the jf-admin on the localhost.");
    }

    CHIP_ERROR Run();

private:
    chip::Optional<chip::CharSpan> ipaddr;
};
