
#include "GrpcServer.h"
#include "JFAdmin.h"

#include <pw_stream/socket_stream.h>
#include <pw_grpc/pw_rpc_handler.h>
#include "pw_rpc_transport/service_registry.h"
#include "pw_thread/thread.h"
#include <pw_thread/test_thread_context.h>
#include "pw_thread_stl/options.h"

#include "rpc_services/JointFabric.h"

#include <lib/support/logging/CHIPLogging.h>

using namespace pw;

static void OnGrpcConnectionClose(void);

static void InitGrpcServer(void);

static void RunGrpcServer(void);

const uint32_t grpcChannelId = 1;
const uint16_t grpcServerPort = 8112;

stream::ServerSocket grpcServerSocket;
grpc::GrpcChannelOutput grpcEgress;
std::array<rpc::Channel, 1> txChannels({rpc::Channel::Create<grpcChannelId>(&grpcEgress)});
pw::rpc::ServiceRegistry serviceRegistry(txChannels);
joint_fabric::JointFabricControlServiceImpl jfControlService;
grpc::PwRpcHandler handler(grpcChannelId, serviceRegistry.client_server().server());

namespace joint_fabric {

pw::Status JointFabricControlServiceImpl::OpenCommissioningWindow(const joint_fabric_OpenCommissioningWindowIn& request,
            joint_fabric_OpenCommissioningWindowOut& response)
{
    /* TODO: Add here the implementation of the OpenCommissioningWindow command */
    ChipLogProgress(NotSpecified, "JointFabricControlService::OpenCommissioningWindow(window_timeout=%u)",
        request.window_timeout);

    return pw::OkStatus();
}

pw::Status JointFabricControlServiceImpl::CommissionDevice(const joint_fabric_CommissionDeviceIn& request,
            joint_fabric_ErrorCode& response)
{
    /* TODO: Add here the implementation of the CommissionDevice command */
    ChipLogProgress(NotSpecified, "JointFabricControlService::CommissionDevice(manual_code=\"%s\", duration=\"%s\")",
        request.manual_code, request.duration);

    JointFabricAdmin::GetInstance().OnboardAdmin(request.manual_code);

    response.err_code = 0;

    return pw::OkStatus();
}

pw::Status JointFabricControlServiceImpl::GetDevices( 
    const pw_protobuf_Empty& request,
    joint_fabric_GetDevicesOut & response)
{
    /* TODO: Add here the implementation of the CommissionDevice command */
    ChipLogProgress(NotSpecified, "JointFabricControlService::GetDevices()");

    return pw::OkStatus();
}

pw::Status JointFabricControlServiceImpl::ControlDevice(
    const joint_fabric_Device& request, 
    joint_fabric_ErrorCode& response)
{
    /* TODO: Add here the implementation of the CommissionDevice command */
    ChipLogProgress(NotSpecified, "JointFabricControlService::ControlDevice(control=\"%lu\")", request.node_id);

    response.err_code = 0;

    return pw::OkStatus();
}

}

static void InitGrpcServer(void)
{
    grpcEgress.set_callbacks(handler);
    serviceRegistry.RegisterService(jfControlService);

    auto status = grpcServerSocket.Listen(grpcServerPort);
    if (!status.ok()) {
        ChipLogError(NotSpecified, "ERROR: Failed to bring up the GRPC server!");
    } else {
        ChipLogProgress(NotSpecified, "GRPC server now listening on port %u.", grpcServerPort);
    }
}

static void OnGrpcConnectionClose(void)
{
    ChipLogProgress(NotSpecified, "GRPC connection close.");
    grpcServerSocket.Close();
}

static void RunGrpcServer(void)
{
    ChipLogProgress(NotSpecified, "Waiting for GRPC connections...");
    auto socket = grpcServerSocket.Accept();
    if (!socket.ok()) {
        ChipLogError(NotSpecified, "ERROR: Failed to accept GRPC connection!");
    while(1) {
    	ChipLogProgress(NotSpecified, "Waiting for GRPC connections...");
    	auto socket = grpcServerSocket.Accept();
    	if (!socket.ok()) {
        	ChipLogError(NotSpecified, "ERROR: Failed to accept GRPC connection!");
    	}
    	ChipLogProgress(NotSpecified, "New GRPC connection accepted.");
	
    	thread::test::TestThreadContext connection_thread_context;
    	thread::test::TestThreadContext send_thread_context;
    	grpc::ConnectionThread conn(
        	*socket,
        	send_thread_context.options(),
        	handler,
        	OnGrpcConnectionClose);
	
    	grpcEgress.set_connection(conn);
	
    	ChipLogProgress(NotSpecified, "Serving GRPC connection.");
    	/* Launch the connection thread */
    	thread::Thread conn_thread(connection_thread_context.options(), conn);
    	conn_thread.join();
    	ChipLogProgress(NotSpecified, "GRPC disconnected.");
	
    	ChipLogProgress(NotSpecified, "New GRPC connection accepted.");
	
    	thread::test::TestThreadContext connection_thread_context;
    	thread::test::TestThreadContext send_thread_context;
    	grpc::ConnectionThread conn(
        	*socket,
        	send_thread_context.options(),
        	handler,
        	OnGrpcConnectionClose);
	
    	grpcEgress.set_connection(conn);
	
    	ChipLogProgress(NotSpecified, "Serving GRPC connection.");
    	/* Launch the connection thread */
    	thread::Thread conn_thread(connection_thread_context.options(), conn);
    	conn_thread.join();
    	ChipLogProgress(NotSpecified, "GRPC disconnected.");
    }
}
	
CHIP_ERROR StartGrpcServer(void)
{
    InitGrpcServer();

    /* Create a thread dedicated to the GRPC server */
    thread::stl::Options options;
    thread::Thread grpcServerThread(options, RunGrpcServer);
    grpcServerThread.detach();

    return CHIP_NO_ERROR;
}
