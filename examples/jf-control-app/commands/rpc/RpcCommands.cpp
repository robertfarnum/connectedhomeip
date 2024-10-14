
#include "RpcCommands.h"

#include <lib/support/logging/CHIPLogging.h>

#include "pw_thread/thread.h"
#include "pw_thread_stl/options.h"

#include "RpcClient.h"
#include "GrpcServer.h"

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

CHIP_ERROR RpcStartGrpcServerCommand::Run()
{
    InitGrpcServer();

    /* Create a thread dedicated to the GRPC server */
    pw::thread::stl::Options options;
    pw::thread::Thread grpcServerThread(options, RunGrpcServer);
    grpcServerThread.detach();

    return CHIP_NO_ERROR;
}
