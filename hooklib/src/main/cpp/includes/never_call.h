//
// Created by swift on 2019/6/3.
//

#ifndef SANDLOCK_NEVER_CALL_H
#define SANDLOCK_NEVER_CALL_H

#include <jni.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_ClassNeverCall_neverCallNative(JNIEnv *env, jobject instance);

#endif //SANDLOCK_NEVER_CALL_H
