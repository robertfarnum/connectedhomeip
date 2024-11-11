
#include "JFAdmin.h"

#include <commands/common/CHIPCommand.h>
#include <commands/interactive/InteractiveCommands.h>

#include "RpcClientProcessor.h"
#include "joint_fabric/joint_fabric.rpc.pb.h"

#define DEFAULT_ONBOARDED_ADMIN_NODE_ID         2

#define RPC_SERVER_PORT         8111
#define DEFAULT_RPC_CHANNEL     1

static void OnOpenCommissioningWindowDone(const ::joint_fabric_ErrorCode &result, ::pw::Status status);

static class JointFabricAdmin jf_admin;
static char rpcServerAddress[48] = { "127.0.0.1" };
static int rpcStatus = RPC_DISCONNECTED;
static volatile bool rpcCallCompleted;

void RpcConnect(void)
{
   CHIP_ERROR err = CHIP_NO_ERROR;

    rpcStatus = RPC_CONNECTING;

    chip::rpc::client::SetRpcServerAddress(rpcServerAddress);
    chip::rpc::client::SetRpcServerPort(RPC_SERVER_PORT);
    err = chip::rpc::client::StartPacketProcessing();

    ChipLogProgress(NotSpecified, "Looking for RPC server @ %s...", rpcServerAddress);
    if (err != CHIP_NO_ERROR) {
        ChipLogProgress(NotSpecified, "Unable to connect!");
        return;
    }

    rpcStatus = RPC_CONNECTED;
    ChipLogProgress(NotSpecified, "Successfully connected to the jf-admin.");
}

void RpcSetServerAddress(const char *addr)
{
    /* TODO: Maybe we should use std strings here */
    memcpy(rpcServerAddress, addr, 48);
    rpcServerAddress[47] = 0;
}

int RpcGetStatus(void)
{
    return rpcStatus;
}

class JointFabricAdmin & JointFabricAdmin::GetInstance(void)
{
    return jf_admin;
}

CHIP_ERROR JointFabricAdmin::OpenCommissioningWindow(uint16_t timeout)
{
    joint_fabric_OpenCommissioningWindowIn request;
    joint_fabric::pw_rpc::nanopb::JointFabric::Client rpcClient(
        chip::rpc::client::GetDefaultRpcClient(),
        DEFAULT_RPC_CHANNEL);

    if (rpcStatus != RPC_CONNECTED) {
        ChipLogError(NotSpecified, "ERROR: Not connected to the jf-admin!");
        return CHIP_ERROR_NOT_CONNECTED;
    }

    /* Populate request */
    request.window_timeout = timeout;

    rpcCallCompleted = false;

    auto call = rpcClient.OpenCommissioningWindow(request, OnOpenCommissioningWindowDone);
    if (!call.active())
    {
        ChipLogError(NotSpecified, "ERROR: Failed to initiate RPC call!");
        return CHIP_ERROR_INTERNAL;
    }

    /* Wait for the RPC call to complete */
    do { } while (!rpcCallCompleted);

    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricAdmin::OnboardAdmin(const char *passcode)
{
    chip::StringBuilder<kMaxCommandSize> cmd;

    cmd.Add("pairing onnetwork-joint-fabric ");
    cmd.AddFormat("%d %s", DEFAULT_ONBOARDED_ADMIN_NODE_ID, passcode);
    /* Note: There is no way to know whether the command was successful or not */
    PushCommand(cmd.c_str());

    return CHIP_NO_ERROR;
}

static void OnOpenCommissioningWindowDone(const ::joint_fabric_ErrorCode &result, ::pw::Status status)
{
    if (status.ok()) {
        ChipLogProgress(NotSpecified, "RPC call successful (server err_code = %d).",
            result.err_code);
    } else {
        ChipLogError(NotSpecified, "ERROR: RPC call failed!");
    }

    rpcCallCompleted = true;
}
