
#include "pigweed/rpc_services/JointFabric.h"
#include <lib/support/logging/CHIPLogging.h>
#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>

using namespace chip;

namespace joint_fabric {

pw::Status JointFabricServiceImpl::OpenCommissioningWindow(const joint_fabric_OpenCommissioningWindowIn& request,
    joint_fabric_OpenCommissioningWindowOut& response)
{
    pw::Status pw_err;
    CHIP_ERROR err(CHIP_NO_ERROR);

    /* Reset response */
    response.manual_code[0] = 0;

    /*
     * TODO: Maybe check that parameters values (window timeout, discriminator,
     * iterations) are in the valid range.
     */
    DeviceLayer::PlatformMgr().LockChipStack();
    switch (request.mode) {
    case 0: /* BCM */
        err = Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow(
            System::Clock::Seconds32(request.window_timeout));
        break;
    case 1: /* ECM */
        DeviceLayer::PlatformMgr().UnlockChipStack();
        return pw::Status::Unimplemented();
    default: /* Not a valid commissioning mode */
        DeviceLayer::PlatformMgr().UnlockChipStack();
        return pw::Status::InvalidArgument();
    }
    DeviceLayer::PlatformMgr().UnlockChipStack();

    /* Check the OCW command response and return the proper error code */
    switch (err.AsInteger()) {
    case CHIP_NO_ERROR.AsInteger(): /* Successful */
        pw_err = pw::OkStatus();
        break;
    default: /* Anything else just return "unknown error" */
        pw_err = pw::Status::Unknown();
        break;
    }

    return pw_err;
}

}
