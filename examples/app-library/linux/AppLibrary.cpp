#include "AppMain.h"

namespace {
constexpr chip::EndpointId kNetworkCommissioningEndpointSecondary = 0xFFFE;
} // anonymous namespace

void ApplicationInit() {}

void ApplicationShutdown() {}

int LIB_StartMatterApp(int argc, char ** argv)
{
    VerifyOrReturnValue(ChipLinuxAppInit(argc, argv) == 0, -1);
    ChipLinuxAppMainLoop();
    return -0;
}
