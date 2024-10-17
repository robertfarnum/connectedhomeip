
#include "pigweed/rpc_services/JointFabric.h"
#include <lib/support/logging/CHIPLogging.h>

namespace joint_fabric {

pw::Status JointFabricServiceImpl::OpenCommissioningWindow(const joint_fabric_OpenCommissioningWindowIn& request,
            joint_fabric_ErrorCode& response)
{
    ChipLogProgress(NotSpecified, "OpenCommissioningWindow(%u)", request.window_timeout);

    response.err_code = 0;

    return pw::OkStatus();
}

}
