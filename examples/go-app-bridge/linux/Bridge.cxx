
#include "bridge/bridge.h"
#include "AppMain.h"

namespace {
constexpr chip::EndpointId kNetworkCommissioningEndpointSecondary = 0xFFFE;
} // anonymous namespace

void ApplicationInit() {}

void ApplicationShutdown() {}

int Bridge::start(int argc, char *argv[])
{
    std::cout << "Bridge::(" << argc << "," << argv << ")" << std::endl;

    if (argv == NULL) {
        argv = (char **)malloc(sizeof(char *));
    }
    VerifyOrReturnValue(ChipLinuxAppInit(argc, argv) == 0, -1);
    ChipLinuxAppMainLoop();
    return -0;
}
