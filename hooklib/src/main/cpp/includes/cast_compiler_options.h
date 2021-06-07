//
// Created by 甘尧 on 2019/1/12.
//

#ifndef SANDLOCK_CAST_COMPILER_OPTIONS_H
#define SANDLOCK_CAST_COMPILER_OPTIONS_H

#include "cast.h"
#include "art_compiler_options.h"

namespace SandLock {

    class CastCompilerOptions {
    public:
        static void init(JNIEnv *jniEnv);
        static IMember<art::CompilerOptions, size_t>* inlineMaxCodeUnits;
    };


}

#endif //SANDLOCK_CAST_COMPILER_OPTIONS_H


