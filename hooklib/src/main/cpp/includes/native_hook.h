//
// Created by SwiftGan on 2019/4/12.
//

#ifndef SANDLOCK_NATIVE_LOCK_H
#define SANDLOCK_NATIVE_LOCK_H

#include "sandhook.h"

namespace SandLock {

    class NativeLock {
    public:
        static bool hookDex2oat(bool disableDex2oat);
    };

}

#endif //SANDLOCK_NATIVE_LOCK_H
