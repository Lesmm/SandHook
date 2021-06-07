//
// Created by SwiftGan on 2019/1/21.
//

#ifndef SANDLOCK_LOCK_H
#define SANDLOCK_LOCK_H

#include "mutex"
#include "../includes/hide_api.h"

namespace SandLock {

    class AutoLock {
    public:
        inline AutoLock(std::mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline AutoLock(std::mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~AutoLock() { mLock.unlock(); }
    private:
        std::mutex& mLock;
    };

    class StopTheWorld {
    public:
        inline StopTheWorld()  { suspendVM(); }
        inline ~StopTheWorld() { resumeVM(); }
    };

}

#endif //SANDLOCK_LOCK_H
