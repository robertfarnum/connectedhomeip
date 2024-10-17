
#include <lib/core/CHIPError.h>

#define RPC_DISCONNECTED        0
#define RPC_CONNECTING          1
#define RPC_CONNECTED           2

void RpcSetServerAddress(const char *addr);

int RpcGetStatus(void);

CHIP_ERROR RpcOpenCommissioningWindow(uint16_t window_timeout);

void RpcConnect(void);
