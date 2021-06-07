#include "includes/sandhook.h"
#include "includes/cast_art_method.h"
#include "includes/trampoline_manager.h"
#include "includes/hide_api.h"
#include "includes/cast_compiler_options.h"
#include "includes/log.h"
#include "includes/native_hook.h"
#include "includes/elf_util.h"
#include "includes/never_call.h"
#include "java_bridge.h"
#include <jni.h>

SandLock::TrampolineManager &trampolineManager = SandLock::TrampolineManager::get();

extern "C" int SDK_INT = 0;
extern "C" bool DEBUG = false;

enum LockMode {
    AUTO = 0,
    INLINE = 1,
    REPLACE = 2
};

LockMode gLockMode = AUTO;

void ensureMethodCached(art::mirror::ArtMethod *hookMethod, art::mirror::ArtMethod *backupMethod) {
    if (SDK_INT >= ANDROID_P)
        return;

    SandLock::StopTheWorld stopTheWorld;

    uint32_t index = backupMethod->getDexMethodIndex();
    if (SDK_INT < ANDROID_O2) {
        hookMethod->setDexCacheResolveItem(index, backupMethod);
    } else {
        int cacheSize = 1024;
        Size slotIndex = index % cacheSize;
        Size newCachedMethodsArray = reinterpret_cast<Size>(calloc(cacheSize, BYTE_POINT * 2));
        unsigned int one = 1;
        memcpy(reinterpret_cast<void *>(newCachedMethodsArray + BYTE_POINT), &one, 4);
        memcpy(reinterpret_cast<void *>(newCachedMethodsArray + BYTE_POINT * 2 * slotIndex),
               (&backupMethod),
               BYTE_POINT
        );
        memcpy(reinterpret_cast<void *>(newCachedMethodsArray + BYTE_POINT * 2 * slotIndex + BYTE_POINT),
               &index,
               4
        );
        hookMethod->setDexCacheResolveList(&newCachedMethodsArray);
    }
}

void ensureDeclareClass(JNIEnv *env, jclass type, jobject originMethod,
                         jobject backupMethod) {
    if (originMethod == NULL || backupMethod == NULL)
        return;
    art::mirror::ArtMethod* origin = getArtMethod(env, originMethod);
    art::mirror::ArtMethod* backup = getArtMethod(env, backupMethod);
    if (origin->getDeclaringClass() != backup->getDeclaringClass()) {
        LOGW("declaring class has been moved!");
        backup->setDeclaringClass(origin->getDeclaringClass());
    }
}

bool doLockWithReplacement(JNIEnv* env,
                           art::mirror::ArtMethod *originMethod,
                           art::mirror::ArtMethod *hookMethod,
                           art::mirror::ArtMethod *backupMethod) {

    if (!hookMethod->compile(env)) {
        hookMethod->disableCompilable();
    }

    if (SDK_INT > ANDROID_N && SDK_INT < ANDROID_Q) {
        forceProcessProfiles();
    }
    if ((SDK_INT >= ANDROID_N && SDK_INT <= ANDROID_P)
        || (SDK_INT >= ANDROID_Q && !originMethod->isAbstract())) {
        originMethod->setHotnessCount(0);
    }

    if (backupMethod != nullptr) {
        originMethod->backup(backupMethod);
        backupMethod->disableCompilable();
        if (!backupMethod->isStatic()) {
            backupMethod->setPrivate();
        }
        backupMethod->flushCache();
    }

    originMethod->disableCompilable();
    hookMethod->disableCompilable();
    hookMethod->flushCache();

    originMethod->disableInterpreterForO();
    originMethod->disableFastInterpreterForQ();

    SandLock::LockTrampoline* hookTrampoline = trampolineManager.installReplacementTrampoline(originMethod, hookMethod, backupMethod);
    if (hookTrampoline != nullptr) {
        originMethod->setQuickCodeEntry(hookTrampoline->replacement->getCode());
        void* entryPointFormInterpreter = hookMethod->getInterpreterCodeEntry();
        if (entryPointFormInterpreter != NULL) {
            originMethod->setInterpreterCodeEntry(entryPointFormInterpreter);
        }
        if (hookTrampoline->callOrigin != nullptr) {
            backupMethod->setQuickCodeEntry(hookTrampoline->callOrigin->getCode());
            backupMethod->flushCache();
        }
        originMethod->flushCache();
        return true;
    } else {
        return false;
    }
}

bool doLockWithInline(JNIEnv* env,
                      art::mirror::ArtMethod *originMethod,
                      art::mirror::ArtMethod *hookMethod,
                      art::mirror::ArtMethod *backupMethod) {

    //fix >= 8.1
    if (!hookMethod->compile(env)) {
        hookMethod->disableCompilable();
    }

    originMethod->disableCompilable();
    if (SDK_INT > ANDROID_N && SDK_INT < ANDROID_Q) {
        forceProcessProfiles();
    }
    if ((SDK_INT >= ANDROID_N && SDK_INT <= ANDROID_P)
        || (SDK_INT >= ANDROID_Q && !originMethod->isAbstract())) {
        originMethod->setHotnessCount(0);
    }
    originMethod->flushCache();

    SandLock::LockTrampoline* hookTrampoline = trampolineManager.installInlineTrampoline(originMethod, hookMethod, backupMethod);

    if (hookTrampoline == nullptr)
        return false;

    hookMethod->flushCache();
    if (hookTrampoline->callOrigin != nullptr) {
        //backup
        originMethod->backup(backupMethod);
        backupMethod->setQuickCodeEntry(hookTrampoline->callOrigin->getCode());
        backupMethod->disableCompilable();
        if (!backupMethod->isStatic()) {
            backupMethod->setPrivate();
        }
        backupMethod->flushCache();
    }
    return true;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_initNative(JNIEnv *env, jclass type, jint sdk, jboolean debug) {
    SDK_INT = sdk;
    DEBUG = debug;
    SandLock::CastCompilerOptions::init(env);
    initHideApi(env);
    SandLock::CastArtMethod::init(env);
    trampolineManager.init(SandLock::CastArtMethod::entryPointQuickCompiled->getOffset());
    return JNI_TRUE;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_swift_sandhook_SandLock_hookMethod(JNIEnv *env, jclass type, jobject originMethod,
                                            jobject hookMethod, jobject backupMethod, jint hookMode) {

    art::mirror::ArtMethod* origin = getArtMethod(env, originMethod);
    art::mirror::ArtMethod* hook = getArtMethod(env, hookMethod);
    art::mirror::ArtMethod* backup = backupMethod == NULL ? nullptr : getArtMethod(env,
                                                                                   backupMethod);

    bool isInlineLock = false;

    int mode = reinterpret_cast<int>(hookMode);

    if (mode == INLINE) {
        if (!origin->isCompiled()) {
            if (SDK_INT >= ANDROID_N) {
                isInlineLock = origin->compile(env);
            }
        } else {
            isInlineLock = true;
        }
        goto label_hook;
    } else if (mode == REPLACE) {
        isInlineLock = false;
        goto label_hook;
    }

    if (origin->isAbstract()) {
        isInlineLock = false;
    } else if (gLockMode != AUTO) {
        if (gLockMode == INLINE) {
            isInlineLock = origin->compile(env);
        } else {
            isInlineLock = false;
        }
    } else if (SDK_INT >= ANDROID_O) {
        isInlineLock = false;
    } else if (!origin->isCompiled()) {
        if (SDK_INT >= ANDROID_N) {
            isInlineLock = origin->compile(env);
        } else {
            isInlineLock = false;
        }
    } else {
        isInlineLock = true;
    }


label_hook:
    //suspend other threads
    SandLock::StopTheWorld stopTheWorld;
    if (isInlineLock && trampolineManager.canSafeInline(origin)) {
        return doLockWithInline(env, origin, hook, backup) ? INLINE : -1;
    } else {
        return doLockWithReplacement(env, origin, hook, backup) ? REPLACE : -1;
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_ensureMethodCached(JNIEnv *env, jclass type, jobject hook,
                                                    jobject backup) {
    art::mirror::ArtMethod* hookeMethod = getArtMethod(env, hook);
    art::mirror::ArtMethod* backupMethod = backup == NULL ? nullptr : getArtMethod(env, backup);
    ensureMethodCached(hookeMethod, backupMethod);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_compileMethod(JNIEnv *env, jclass type, jobject member) {

    if (member == NULL)
        return JNI_FALSE;
    art::mirror::ArtMethod* method = getArtMethod(env, member);

    if (method == nullptr)
        return JNI_FALSE;

    if (!method->isCompiled()) {
        SandLock::StopTheWorld stopTheWorld;
        if (!method->compile(env)) {
            if (SDK_INT >= ANDROID_N) {
                method->disableCompilable();
                method->flushCache();
            }
            return JNI_FALSE;
        } else {
            return JNI_TRUE;
        }
    } else {
        return JNI_TRUE;
    }

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_deCompileMethod(JNIEnv *env, jclass type, jobject member, jboolean disableJit) {

    if (member == NULL)
        return JNI_FALSE;
    art::mirror::ArtMethod* method = getArtMethod(env, member);

    if (method == nullptr)
        return JNI_FALSE;

    if (disableJit) {
        method->disableCompilable();
    }

    if (method->isCompiled()) {
        SandLock::StopTheWorld stopTheWorld;
        if (SDK_INT >= ANDROID_N) {
            method->disableCompilable();
        }
        return static_cast<jboolean>(method->deCompile());
    } else {
        return JNI_TRUE;
    }

}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_swift_sandhook_SandLock_getObjectNative(JNIEnv *env, jclass type, jlong thread,
                                                 jlong address) {
    return getJavaObject(env, thread ? reinterpret_cast<void *>(thread) : getCurrentThread(), reinterpret_cast<void *>(address));
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_canGetObject(JNIEnv *env, jclass type) {
    return static_cast<jboolean>(canGetObject());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_setLockMode(JNIEnv *env, jclass type, jint mode) {
    gLockMode = static_cast<LockMode>(mode);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_setInlineSafeCheck(JNIEnv *env, jclass type, jboolean check) {
    trampolineManager.inlineSecurityCheck = check;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_skipAllSafeCheck(JNIEnv *env, jclass type, jboolean skip) {
    trampolineManager.skipAllCheck = skip;
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_is64Bit(JNIEnv *env, jclass type) {
    return static_cast<jboolean>(BYTE_POINT == 8);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_disableVMInline(JNIEnv *env, jclass type) {
    if (SDK_INT < ANDROID_N)
        return JNI_FALSE;
    replaceUpdateCompilerOptionsQ();
    art::CompilerOptions* compilerOptions = getGlobalCompilerOptions();
    if (compilerOptions == nullptr)
        return JNI_FALSE;
    return static_cast<jboolean>(disableJitInline(compilerOptions));
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_disableDex2oatInline(JNIEnv *env, jclass type, jboolean disableDex2oat) {
    return static_cast<jboolean>(SandLock::NativeLock::hookDex2oat(disableDex2oat));
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_setNativeEntry(JNIEnv *env, jclass type, jobject origin, jobject hook, jlong jniTrampoline) {
    if (origin == nullptr || hook == NULL)
        return JNI_FALSE;
    art::mirror::ArtMethod* hookMethod = getArtMethod(env, hook);
    art::mirror::ArtMethod* originMethod = getArtMethod(env, origin);
    originMethod->backup(hookMethod);
    hookMethod->setNative();
    hookMethod->setQuickCodeEntry(SandLock::CastArtMethod::genericJniStub);
    hookMethod->setJniCodeEntry(reinterpret_cast<void *>(jniTrampoline));
    hookMethod->disableCompilable();
    hookMethod->flushCache();
    return JNI_TRUE;
}


static jclass class_pending_hook = nullptr;
static jmethodID method_class_init = nullptr;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_swift_sandhook_SandLock_initForPendingLock(JNIEnv *env, jclass type) {
    class_pending_hook = static_cast<jclass>(env->NewGlobalRef(__FindClassEx__(env, kCLASS_PENDING)));
    method_class_init = env->GetStaticMethodID(class_pending_hook, kMETHOD_PENDING_INIT, kMETHOD_PENDING_INIT_SIG);
    auto class_init_handler = [](void *clazz_ptr) {
        attachAndGetEvn()->CallStaticVoidMethod(class_pending_hook, method_class_init, (jlong) clazz_ptr);
        attachAndGetEvn()->ExceptionClear();
    };
    return static_cast<jboolean>(hookClassInit(class_init_handler));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_ClassNeverCall_neverCallNative(JNIEnv *env, jobject instance) {
    int a = 1 + 1;
    int b = a + 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_ClassNeverCall_neverCallNative2(JNIEnv *env, jobject instance) {
    int a = 4 + 3;
    int b = 9 + 6;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_test_TestClass_jni_1test(JNIEnv *env, jobject instance) {
    int a = 1 + 1;
    int b = a + 1;
}

//native hook
extern "C"
JNIEXPORT bool nativeLockNoBackup(void* origin, void* hook) {

    if (origin == nullptr || hook == nullptr)
        return false;

    SandLock::StopTheWorld stopTheWorld;

    return trampolineManager.installNativeLockTrampolineNoBackup(origin, hook) != nullptr;

}
extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_MakeInitializedClassVisibilyInitialized(JNIEnv *env, jclass clazz,
                                                                         jlong self) {
    MakeInitializedClassVisibilyInitialized(reinterpret_cast<void*>(self));
}
extern "C"
JNIEXPORT void* findSym(const char *elf, const char *sym_name) {
    SandLock::ElfImg elfImg(elf);
    return reinterpret_cast<void *>(elfImg.getSymbAddress(sym_name));
}

static JNINativeMethod jniSandLock[] = {
        {
                "initNative",
                "(IZ)Z",
                (void *) Java_com_swift_sandhook_SandLock_initNative
        },
        {
                "hookMethod",
                "(Ljava/lang/reflect/Member;Ljava/lang/reflect/Method;Ljava/lang/reflect/Method;I)I",
                (void *) Java_com_swift_sandhook_SandLock_hookMethod
        },
        {
                "ensureMethodCached",
                "(Ljava/lang/reflect/Method;Ljava/lang/reflect/Method;)V",
                (void *) Java_com_swift_sandhook_SandLock_ensureMethodCached
        },
        {
                "ensureDeclareClass",
                "(Ljava/lang/reflect/Member;Ljava/lang/reflect/Method;)V",
                (void *) ensureDeclareClass
        },
        {
                "compileMethod",
                "(Ljava/lang/reflect/Member;)Z",
                (void *) Java_com_swift_sandhook_SandLock_compileMethod
        },
        {
                "deCompileMethod",
                "(Ljava/lang/reflect/Member;Z)Z",
                (void *) Java_com_swift_sandhook_SandLock_deCompileMethod
        },
        {
                "getObjectNative",
                "(JJ)Ljava/lang/Object;",
                (void *) Java_com_swift_sandhook_SandLock_getObjectNative
        },
        {
                "canGetObject",
                "()Z",
                (void *) Java_com_swift_sandhook_SandLock_canGetObject
        },
        {
                "setLockMode",
                "(I)V",
                (void *) Java_com_swift_sandhook_SandLock_setLockMode
        },
        {
                "setInlineSafeCheck",
                "(Z)V",
                (void *) Java_com_swift_sandhook_SandLock_setInlineSafeCheck
        },
        {
                "skipAllSafeCheck",
                "(Z)V",
                (void *) Java_com_swift_sandhook_SandLock_skipAllSafeCheck
        },
        {
                "is64Bit",
                "()Z",
                (void *) Java_com_swift_sandhook_SandLock_is64Bit
        },
        {
                "disableVMInline",
                "()Z",
                (void *) Java_com_swift_sandhook_SandLock_disableVMInline
        },
        {
                "disableDex2oatInline",
                "(Z)Z",
                (void *) Java_com_swift_sandhook_SandLock_disableDex2oatInline
        },
        {
                "setNativeEntry",
                "(Ljava/lang/reflect/Member;Ljava/lang/reflect/Member;J)Z",
                (void *) Java_com_swift_sandhook_SandLock_setNativeEntry
        },
        {
                "initForPendingLock",
                "()Z",
                (void *) Java_com_swift_sandhook_SandLock_initForPendingLock
        },
        {
                "MakeInitializedClassVisibilyInitialized",
                "(J)V",
                (void*) Java_com_swift_sandhook_SandLock_MakeInitializedClassVisibilyInitialized
        },
};

static JNINativeMethod jniNeverCall[] = {
        {
                "neverCallNative",
                "()V",
                (void *) Java_com_swift_sandhook_ClassNeverCall_neverCallNative
        },
        {
                "neverCallNative2",
                "()V",
                (void *) Java_com_swift_sandhook_ClassNeverCall_neverCallNative2
        }
};

static bool registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *jniMethods, int methods) {
    jclass clazz = __FindClassEx__(env, className);
    if (clazz == NULL) {
        return false;
    }
    jint status = env->RegisterNatives(clazz, jniMethods, methods);
    bool isOk = status >= 0;
    return isOk;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_swift_sandhook_SandLock_test(JNIEnv *env, jclass clazz, jobject class_loader, jobjectArray classes_methods_fields) {
    __FillVariables__(env, class_loader, classes_methods_fields);

    int jniMethodSize = sizeof(JNINativeMethod);
    if (!registerNativeMethods(env, kCLASS_SAND, jniSandLock, sizeof(jniSandLock) / jniMethodSize)) {
        LOGE("JNI __JNI_OnLoad__ error kCLASS_SAND");
        return;
    }

    if (!registerNativeMethods(env, kCLASS_NEVER, jniNeverCall, sizeof(jniNeverCall) / jniMethodSize)) {
        LOGE("JNI __JNI_OnLoad__ error kCLASS_NEVER");
        return;
    }
    LOGD("JNI __JNI_OnLoad__ success");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {

    JNIEnv *env = NULL;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    JNINativeMethod jniTest[] = {
            {
                    "test",
                    "(Ljava/lang/ClassLoader;[Ljava/lang/String;)V",
                    (void*) Java_com_swift_sandhook_SandLock_test
            }
    };

    if (!registerNativeMethods(env, "com/swift/sandhook/SandHook", jniTest, 1)) {
        return -1;
    }

//    if (!registerNativeMethods(env, kCLASS_NEVER, jniNeverCall, sizeof(jniNeverCall) / jniMethodSize)) {
//        return -1;
//    }

    LOGW("JNI Loaded");

    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT bool JNI_Load_Ex(JNIEnv* env, jclass classSandLock, jclass classNeverCall) {
    int jniMethodSize = sizeof(JNINativeMethod);

    if (env == nullptr || classSandLock == nullptr || classNeverCall == nullptr)
        return false;

    if (env->RegisterNatives(classSandLock, jniSandLock, sizeof(jniSandLock) / jniMethodSize) < 0) {
        return false;
    }

    if (env->RegisterNatives(classNeverCall, jniNeverCall, sizeof(jniNeverCall) / jniMethodSize) < 0) {
        return false;
    }

    LOGW("JNI Loaded");
    return true;
}
