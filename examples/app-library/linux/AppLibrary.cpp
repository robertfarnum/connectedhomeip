#include "AppMain.h"

namespace {
constexpr chip::EndpointId kNetworkCommissioningEndpointSecondary = 0xFFFE;
} // anonymous namespace

void ApplicationInit() {}

void ApplicationShutdown() {}

class Bridge
{
public:
    virtual ~Bridge() { std::cout << "Bridge::~Bridge()" << std::endl; }
    virtual void cb() { std::cout << "Bridge::cb()" << std::endl; }
};

int start(int argc, char **argv)
{
    VerifyOrReturnValue(ChipLinuxAppInit(argc, argv) == 0, -1);
    ChipLinuxAppMainLoop();
    return -0;
}
