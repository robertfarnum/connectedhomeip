
#ifndef __JFADMIN_H__
#define __JFADMIN_H__

#include <lib/core/CHIPError.h>

class JointFabricAdmin;

class JointFabricAdmin {
public:
    static class JointFabricAdmin & GetInstance(void);

    CHIP_ERROR OpenCommissioningWindow(uint16_t timeout);

    CHIP_ERROR OnboardAdmin(const char *passcode);
};

#endif /* __JFADMIN_H__ */
