
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
