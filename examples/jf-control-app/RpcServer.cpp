
#include "RpcServer.h"

#include "pw_rpc_system_server/rpc_server.h"
#include "pw_rpc_system_server/socket.h"
#include "pw_thread/thread.h"
#include "pw_thread_stl/options.h"

#include <lib/support/logging/CHIPLogging.h>

#include "joint_fabric/joint_fabric.rpc.pb.h"

#define RPC_SERVER_PORT         8113

using namespace chip;

namespace joint_fabric {

class TestImpl : public pw_rpc::nanopb::Test::Service<TestImpl> {
 public:
  pw::Status DisplayText(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response);
};

class ReverseJointFabricServiceImpl : public pw_rpc::nanopb::ReverseJointFabric::Service<ReverseJointFabricServiceImpl> {
 public:
  pw::Status UpdateOperationalIdentity(const pw_protobuf_Empty& request, joint_fabric_ErrorCode& response);
};

pw::Status TestImpl::DisplayText(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response)
{
    /* Display the provided text string */
    ChipLogProgress(NotSpecified, "jf-admin has sent a new message: \"%s\"", request.msg_data);

    response.err_code = 0;

    return pw::OkStatus();
}

pw::Status ReverseJointFabricServiceImpl::UpdateOperationalIdentity(const pw_protobuf_Empty& request, joint_fabric_ErrorCode& response)
{
    ChipLogProgress(NotSpecified, "UpdateOperationalIdentity()");
    return pw::OkStatus();
}

}

static void RpcServerRun(void);

static void RpcServerRun(void)
{
    joint_fabric::ReverseJointFabricServiceImpl reverse_joint_fabric_service;

    pw::rpc::system_server::set_socket_port(RPC_SERVER_PORT);
    pw::rpc::system_server::Server().RegisterService(reverse_joint_fabric_service);
    ChipLogProgress(NotSpecified, "RPC server now listening on port %d.", RPC_SERVER_PORT);
    pw::rpc::system_server::Init();
    ChipLogProgress(NotSpecified, "jf-admin connected.");

    pw::rpc::system_server::Start();
}

CHIP_ERROR RpcServerStart(void)
{
    /* Create a thread dedicated to the RPC server */
    pw::thread::stl::Options options;
    pw::thread::Thread rpcServerThread(options, RpcServerRun);
    rpcServerThread.detach();

    return CHIP_NO_ERROR;
}
