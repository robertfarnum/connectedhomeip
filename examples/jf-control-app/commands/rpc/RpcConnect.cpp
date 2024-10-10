
#include "RpcConnect.h"

#include <lib/support/logging/CHIPLogging.h>

#include "RpcClient.h"

CHIP_ERROR RpcConnectCommand::RunCommand()
{
    RpcConnect();
    
    return CHIP_NO_ERROR;
}
