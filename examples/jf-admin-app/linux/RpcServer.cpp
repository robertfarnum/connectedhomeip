
#include "pigweed/rpc_services/JointFabric.h"
#include <lib/support/logging/CHIPLogging.h>

namespace test {

pw::Status TestImpl::DisplayText(const test_TextMessage& request, test_ErrorCode& response) {
    /* Display the provided text string */
    ChipLogProgress(NotSpecified, "New message received: \"%s\"", request.msg_data);
    /* Return a recognizable error code, just for fun */
    response.err_code = 107;

    return pw::OkStatus();
}

}

