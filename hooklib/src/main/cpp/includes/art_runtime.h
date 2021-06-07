//
// Created by 甘尧 on 2019/2/23.
//

#ifndef SANDLOCK_ART_RUNTIME_H
#define SANDLOCK_ART_RUNTIME_H

#include "art_jit.h"

namespace art {
    class Runtime {

    public:
        jit::Jit* getJit();
    };
}

#endif //SANDLOCK_ART_RUNTIME_H
