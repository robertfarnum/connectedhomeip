
#include "RpcClient.h"

#include <lib/support/logging/CHIPLogging.h>

#include "RpcClientProcessor.h"
#include "joint_fabric/joint_fabric.rpc.pb.h"

#define RPC_SERVER_PORT                 8113
#define DEFAULT_RPC_CHANNEL             1

using namespace pw;

static void OnRpcCallCompleted(const ::joint_fabric_ErrorCode &result, ::pw::Status status);

static volatile bool rpcCallCompleted;

CHIP_ERROR RpcConnect(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    chip::rpc::client::SetRpcServerAddress("127.0.0.1");
    chip::rpc::client::SetRpcServerPort(RPC_SERVER_PORT);
    err = chip::rpc::client::StartPacketProcessing();

    if (err != CHIP_NO_ERROR) {
        return CHIP_ERROR_CONNECTION_ABORTED;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR RpcDisplayText(const char *message)
{
    ::joint_fabric_TextMessage request;
    joint_fabric::pw_rpc::nanopb::Test::Client rpcClient(
        chip::rpc::client::GetDefaultRpcClient(),
        DEFAULT_RPC_CHANNEL);

    ChipLogProgress(NotSpecified, "Sending message via RPC.");

    /* Populate request */
    strcpy(request.msg_data, message);

    rpcCallCompleted = false;

    auto call = rpcClient.DisplayText(request, OnRpcCallCompleted);
    if (!call.active())
    {
        ChipLogError(NotSpecified, "RPC call to control plane has failed!");
        return CHIP_ERROR_INTERNAL;
    }

    /* Wait for the RPC call to complete */
    do { } while (!rpcCallCompleted);

    return CHIP_NO_ERROR;
}

static void OnRpcCallCompleted(const ::joint_fabric_ErrorCode &result, ::pw::Status status)
{
    if (status.ok()) {
        ChipLogProgress(NotSpecified, "RPC call successful (server err_code = %d).",
            result.err_code);
    } else {
        ChipLogError(NotSpecified, "RPC call to control plane has failed!");
    }

    rpcCallCompleted = true;
}
