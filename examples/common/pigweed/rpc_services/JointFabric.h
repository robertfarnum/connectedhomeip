#pragma once

#include "joint_fabric_service/joint_fabric_service.rpc.pb.h"
#include <crypto/CHIPCryptoPAL.h>
#include <platform/CHIPDeviceLayer.h>

namespace joint_fabric_service {

class JointFabric : public pw_rpc::nanopb::JointFabric::Service<JointFabric>
{
public:
    /*RPC from jfc-client to jfa-server */
    ::pw::Status IssueIcac(const ::IssueIcacRequest & request, ::IssueIcacResponse & response);
    ::pw::Status StartJcm(const ::StartJcmRequest & request, ::pw_protobuf_Empty & response);

private:
    static void FinalizeCommissioningWork(intptr_t arg);
};

} // namespace joint_fabric_service
