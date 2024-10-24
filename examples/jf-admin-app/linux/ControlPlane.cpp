
#include "ControlPlane.h"
#include "RpcClient.h"

class JointFabricControl jf_control;

class JointFabricControl & JointFabricControl::GetInstance(void)
{
    return jf_control;
}

CHIP_ERROR JointFabricControl::UpdateOperationalIdentity(chip::NodeId nodeId)
{
    return RpcUpdateOperationalIdentity(nodeId);
}
