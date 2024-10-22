
#include "JFAdmin.h"
#include "RpcClient.h"

class JointFabricAdmin jf_admin;

class JointFabricAdmin & JointFabricAdmin::GetInstance(void)
{
    return jf_admin;
}

CHIP_ERROR JointFabricAdmin::OpenCommissioningWindow(uint16_t timeout)
{
    return RpcOpenCommissioningWindow(timeout);
}
