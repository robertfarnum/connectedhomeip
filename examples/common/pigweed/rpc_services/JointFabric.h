
#include "test/test.rpc.pb.h"

namespace joint_fabric {

class TestImpl final: public pw_rpc::nanopb::Test::Service<TestImpl> {
    public:
        pw::Status DisplayText(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response);
};

class JointFabricServiceImpl : public pw_rpc::nanopb::JointFabric::Service<JointFabricServiceImpl> {
    public:
        pw::Status OpenCommissioningWindow(const ::joint_fabric_OpenCommissioningWindowIn& request,
            joint_fabric_ErrorCode& response);
};

class JointFabricControlServiceImpl : public pw_rpc::nanopb::JointFabricControl::Service<JointFabricControlServiceImpl> {
    public:
        pw::Status OpenCommissioningWindow(const joint_fabric_OpenCommissioningWindowIn& request,
            joint_fabric_OpenCommissioningWindowOut& response);
        pw::Status CommissionDevice(const joint_fabric_CommissionDeviceIn& request,
            joint_fabric_ErrorCode& response);
};

}
