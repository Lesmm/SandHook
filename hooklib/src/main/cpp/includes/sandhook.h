//
// Created by SwiftGan on 2019/4/12.
//

#ifndef SANDLOCK_SANDLOCK_H
#define SANDLOCK_SANDLOCK_H

#include <jni.h>

extern "C"
JNIEXPORT bool nativeLockNoBackup(void* origin, void* hook);

extern "C"
JNIEXPORT void* findSym(const char *elf, const char *sym_name);

#endif //SANDLOCK_SANDLOCK_H
