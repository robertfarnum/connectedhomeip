
#ifndef __CONTROLPLANE_H__
#define __CONTROLPLANE_H__

#include <lib/core/CHIPError.h>
#include <lib/core/NodeId.h>

class JointFabricControl;

class JointFabricControl {
public:
    static class JointFabricControl & GetInstance(void);

    CHIP_ERROR UpdateOperationalIdentity(chip::NodeId nodeId);
};

#endif /* __CONTROLPLANE_H__ */
