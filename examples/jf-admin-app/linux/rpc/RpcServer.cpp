#include "pigweed/rpc_services/JointFabric.h"

#include "JFAManager.h"

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;

namespace joint_fabric_service {

::pw::Status JointFabric::IssueIcac(const ::IssueIcacRequest & request, ::IssueIcacResponse & response)
{
    ChipLogProgress(JointFabric, "RPC IssueIcac");

    // Get the ICAC CSR from request
    ByteSpan icacCsr(request.icac_csr.bytes, request.icac_csr.size);
    FabricId anchorFabricId = request.anchor_fabric_id;

    // Issue ICAC using the JFAManager's OperationalCredentialsIssuer
    uint8_t icacBuf[chip::Credentials::kMaxDERCertLength];
    MutableByteSpan icac(icacBuf);
    
    CHIP_ERROR err = JFAMgr().SignIcac(icacCsr, anchorFabricId, icac);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(JointFabric, "SignICAC Failed");
        return pw::Status::Internal();
    }

    // Populate response
    memcpy(response.icac.bytes, icac.data(), icac.size());
    response.icac.size = icac.size();

    return pw::OkStatus();
}

::pw::Status JointFabric::StartJcm(const ::StartJcmRequest & request, ::pw_protobuf_Empty & response)
{
    ChipLogProgress(JointFabric, "RPC StartJcm for NodeId: 0x" ChipLogFormatX64 " peerEndpointId: %u",
                    ChipLogValueX64(request.node_id), static_cast<unsigned>(request.peer_admin_endpoint_id));

    // Schedule JCM start on the Matter event loop
    struct StartJcmArgs
    {
        uint64_t nodeId;
        uint32_t peerAdminEndpointId;
    };

    StartJcmArgs * args = Platform::New<StartJcmArgs>();
    VerifyOrReturnValue(args, pw::Status::Internal());
    args->nodeId               = request.node_id;
    args->peerAdminEndpointId  = request.peer_admin_endpoint_id;

    TEMPORARY_RETURN_IGNORED DeviceLayer::PlatformMgr().ScheduleWork(FinalizeCommissioningWork, reinterpret_cast<intptr_t>(args));

    return pw::OkStatus();
}

void JointFabric::FinalizeCommissioningWork(intptr_t arg)
{
    struct StartJcmArgs
    {
        uint64_t nodeId;
        uint32_t peerAdminEndpointId;
    };

    StartJcmArgs * args = reinterpret_cast<StartJcmArgs *>(arg);
    TEMPORARY_RETURN_IGNORED JFAMgr().StartJcm(static_cast<NodeId>(args->nodeId),
                                               static_cast<EndpointId>(args->peerAdminEndpointId));
    chip::Platform::Delete(args);
}

} // namespace joint_fabric_service
