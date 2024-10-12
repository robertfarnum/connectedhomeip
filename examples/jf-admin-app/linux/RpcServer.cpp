
#include "pigweed/rpc_services/JointFabric.h"
#include <lib/support/logging/CHIPLogging.h>

namespace joint_fabric {

pw::Status TestImpl::DisplayText(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response)
{
    /* Display the provided text string */
    ChipLogProgress(NotSpecified, "New message received: \"%s\"", request.msg_data);
    /* Return a recognizable error code, just for fun */
    response.err_code = 107;

    return pw::OkStatus();
}

pw::Status JointFabricServiceImpl::OpenCommissioningWindow(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response)
{
    return pw::OkStatus();
}

pw::Status JointFabricControlServiceImpl::OpenCommissioningWindow(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response)
{
    return pw::OkStatus();
}

pw::Status JointFabricControlServiceImpl::CommissionDevice(const joint_fabric_TextMessage& request, joint_fabric_ErrorCode& response)
{
    return pw::OkStatus();
}

}

