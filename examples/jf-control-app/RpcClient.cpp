
#include "RpcClient.h"

#include <pw_chrono/system_timer.h>

#include <lib/support/logging/CHIPLogging.h>

#include "RpcClientProcessor.h"
#include "test/test.rpc.pb.h"

#define RPC_SERVER_PORT         8111
#define DEFAULT_RPC_CHANNEL     1

using namespace pw;

static void OnRpcConnect(chrono::SystemClock::time_point expired_deadline);

static void OnRpcCallCompleted(const ::test_ErrorCode &result, ::pw::Status status);

static chrono::SystemTimer timer(OnRpcConnect);

static char rpcServerAddress[48] = { "127.0.0.1" };
static int rpcStatus = RPC_DISCONNECTED;
static volatile bool rpcCallCompleted;

void RpcConnect(void)
{
    rpcStatus = RPC_CONNECTING;

    /* Fire up the timer which connects the RPC */
    timer.InvokeAfter(std::chrono::seconds(1));
}

void RpcSetServerAddress(const char *addr)
{
    /* TODO: Must use std strings here */
    strcpy(rpcServerAddress, addr);
}

int RpcGetStatus(void)
{
    return rpcStatus;
}

CHIP_ERROR RpcDisplayText(const char *message)
{
    ::test_TextMessage request;
    test::pw_rpc::nanopb::Test::Client rpcClient(
        chip::rpc::client::GetDefaultRpcClient(),
        DEFAULT_RPC_CHANNEL);

    if (rpcStatus != RPC_CONNECTED) {
        ChipLogError(NotSpecified, "ERROR: Not connected to the RPC server!");
        return CHIP_ERROR_NOT_CONNECTED;
    }

    ChipLogProgress(NotSpecified, "Sending message via RPC.");

    /* Populate request */
    strcpy(request.msg_data, message);

    rpcCallCompleted = false;

    auto call = rpcClient.DisplayText(request, OnRpcCallCompleted);
    if (!call.active())
    {
        ChipLogError(NotSpecified, "ERROR: Failed to initiate RPC call!");
        return CHIP_ERROR_INTERNAL;
    }

    /* Wait for the RPC call to complete */
    do { } while (!rpcCallCompleted);

    return CHIP_NO_ERROR;
}

static void OnRpcConnect(chrono::SystemClock::time_point expired_deadline)
{
    CHIP_ERROR err;

    chip::rpc::client::SetRpcServerAddress(rpcServerAddress);
    chip::rpc::client::SetRpcServerPort(RPC_SERVER_PORT);
    err = chip::rpc::client::StartPacketProcessing();

    if (err != CHIP_NO_ERROR) {
        ChipLogProgress(NotSpecified, "Unable to connect to RPC server %s! Trying again in 5 seconds.",
            rpcServerAddress);
        /* Reschedule connect timer after another 3 seconds */
        timer.InvokeAfter(std::chrono::seconds(5));
    } else {
        rpcStatus = RPC_CONNECTED;
        ChipLogProgress(NotSpecified, "Successfully connected to the jf-admin.");
    }
}

static void OnRpcCallCompleted(const ::test_ErrorCode &result, ::pw::Status status)
{
    if (status.ok()) {
        ChipLogProgress(NotSpecified, "RPC call successful (server err_code = %d).",
            result.err_code);
    } else {
        ChipLogError(NotSpecified, "ERROR: RPC call failed!");
    }

    rpcCallCompleted = true;
}
