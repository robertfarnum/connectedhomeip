
#ifndef __CONTROLPLANE_H__
#define __CONTROLPLANE_H__

#include <lib/core/CHIPError.h>

class JointFabricControl;

class JointFabricControl {
public:
    static class JointFabricControl & GetInstance(void);

    CHIP_ERROR UpdateOperationalIdentity(void);
};

#endif /* __CONTROLPLANE_H__ */
