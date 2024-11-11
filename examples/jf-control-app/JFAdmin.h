
#ifndef __JFADMIN_H__
#define __JFADMIN_H__

#include <json/json.h>
#include <lib/core/CHIPError.h>

#define RPC_DISCONNECTED        0
#define RPC_CONNECTING          1
#define RPC_CONNECTED           2

class JointFabricAdmin;

class JointFabricAdmin {
public:
    static class JointFabricAdmin & GetInstance(void);

    // BCM
    CHIP_ERROR OpenCommissioningWindow(uint16_t timeout);

    // ECM
    CHIP_ERROR OpenCommissioningWindow(uint16_t timeout, uint32_t iterations, uint16_t discriminator, Json::Value * result = NULL);

    CHIP_ERROR OnboardAdmin(const char *passcode);
};

void RpcSetServerAddress(const char *addr);

int RpcGetStatus(void);

void RpcConnect(void);

#endif /* __JFADMIN_H__ */
