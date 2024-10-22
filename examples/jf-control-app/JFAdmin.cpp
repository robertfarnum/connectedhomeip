
#include "JFAdmin.h"

#include <commands/common/CHIPCommand.h>
#include <commands/interactive/InteractiveCommands.h>

#include "RpcClient.h"

#define DEFAULT_ONBOARDED_ADMIN_NODE_ID         1

class JointFabricAdmin jf_admin;

class JointFabricAdmin & JointFabricAdmin::GetInstance(void)
{
    return jf_admin;
}

CHIP_ERROR JointFabricAdmin::OpenCommissioningWindow(uint16_t timeout)
{
    return RpcOpenCommissioningWindow(timeout);
}

CHIP_ERROR JointFabricAdmin::OnboardAdmin(const char *passcode)
{
    chip::StringBuilder<kMaxCommandSize> cmd;

    cmd.Add("pairing onnetwork-joint-fabric ");
    cmd.AddFormat("%d %s", DEFAULT_ONBOARDED_ADMIN_NODE_ID, passcode);
    /* Note: There is no way to know whether the command was successful or not */
    PushCommand(cmd.c_str());

    return CHIP_NO_ERROR;
}
