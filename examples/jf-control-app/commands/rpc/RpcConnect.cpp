
#include "RpcConnect.h"

#include <lib/support/logging/CHIPLogging.h>

#include "JFAdmin.h"

CHIP_ERROR RpcConnectCommand::Run()
{
    if (ipaddr.HasValue()) {
        RpcSetServerAddress(ipaddr.Value().data());
    }
    RpcConnect();

    return CHIP_NO_ERROR;
}
