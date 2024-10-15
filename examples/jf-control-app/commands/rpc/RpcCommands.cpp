
#include "RpcCommands.h"

#include <lib/support/logging/CHIPLogging.h>

#include "RpcClient.h"

#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
#include "pw_thread/thread.h"
#include "pw_thread_stl/options.h"

#include "GrpcServer.h"
#endif /* CONFIG_ENABLE_GRPC */

#define MAX_MESSAGE_LEN         65

CHIP_ERROR RpcSendCommand::RunCommand(chip::CharSpan & msg)
{
    char m_data[MAX_MESSAGE_LEN] = { 0 };
    chip::MutableCharSpan m(m_data);

    CopyCharSpanToMutableCharSpanWithTruncation(msg, m);

    /*
     * The "CopyCharSpanToMutableCharSpanWithTruncation" is known to overwrite
     * the ending null terminator of the text string when 'msg' is longer than
     * 'm'.
     */
    if (msg.size() >= MAX_MESSAGE_LEN) {
        m[MAX_MESSAGE_LEN - 1] = 0;
    }

    return RpcDisplayText(m.data());
}

#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
CHIP_ERROR RpcStartGrpcServerCommand::Run()
{
    InitGrpcServer();

    /* Create a thread dedicated to the GRPC server */
    pw::thread::stl::Options options;
    pw::thread::Thread grpcServerThread(options, RunGrpcServer);
    grpcServerThread.detach();

    return CHIP_NO_ERROR;
}
#endif /* CONFIG_ENABLE_GRPC */

CHIP_ERROR RpcAdminAddrCommand::Run()
{
    RpcSetServerAddress(ipaddr.data());
    return CHIP_NO_ERROR;
}

CHIP_ERROR RpcStatusCommand::Run()
{
    int rpcStatus = RpcGetStatus();

    switch(rpcStatus) {
    case RPC_DISCONNECTED:
        ChipLogProgress(NotSpecified, "jf-admin NOT connected.");
        break;
    case RPC_CONNECTING:
        ChipLogProgress(NotSpecified, "Still searching for jf-admin.");
        break;
    case RPC_CONNECTED:
        ChipLogProgress(NotSpecified, "jf-admin connected.");
        break;
    default:
        ChipLogError(NotSpecified, "ERROR: Unknown RPC status (code=%d)!", rpcStatus);
        break;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR RpcOpenCommissioningWindowCommand::Run()
{
    return RpcOpenCommissioningWindow(window_timeout);
}
