
#include "pigweed/rpc_services/JointFabric.h"
#include <lib/support/logging/CHIPLogging.h>

namespace joint_fabric {

pw::Status JointFabricServiceImpl::OpenCommissioningWindow(const joint_fabric_OpenCommissioningWindowIn& request,
    joint_fabric_OpenCommissioningWindowOut& response)
{
    response.manual_code[0] = 0;

    ChipLogProgress(NotSpecified, "OpenCommissioningWindow() call from control plane");
    ChipLogProgress(NotSpecified, "    - mode = %u", request.mode);
    ChipLogProgress(NotSpecified, "    - window_timeout = %u", request.window_timeout);
    if (request.mode == 1) {
        ChipLogProgress(NotSpecified, "    - iterations = %u", request.iterations);
        ChipLogProgress(NotSpecified, "    - discriminator = %u", request.discriminator);
    }

    return pw::OkStatus();
}

}
