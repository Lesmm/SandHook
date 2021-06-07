//
// Created by swift on 2019/5/23.
//

#pragma once

#include "hook.h"
#include <vector>

namespace SandLock {
    namespace Lock {

        class InlineLockArm32Android : public InlineLock {
        public:
            void *Lock(void *origin, void *replace) override;

            bool BreakPoint(void *point, void (*callback)(REG *)) override;

            bool SingleBreakPoint(void *point, BreakCallback callback, void *data) override;

            void *SingleInstLock(void *origin, void *replace) override;

            bool ExceptionHandler(int num, sigcontext *context) override;

        private:
            std::vector<LockInfo> hook_infos;
        };

    }
}