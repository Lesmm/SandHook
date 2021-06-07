//
// Created by ubuntu on 5/31/21.
//

#include "java_bridge.h"
#include "includes/log.h"
#include "includes/dlfcn_nougat.h"

const char *kCLASS_SAND;
const char *kMETHOD_SAND_GETART;
const char *kMETHOD_SAND_GETART_SIG;
const char *kMETHOD_SAND_GETJAVA;
const char *kMETHOD_SAND_GETJAVA_SIG;
const char *kMETHOD_SAND_GETTHREAD;
const char *kFIELD_SAND_ACCESS;

const char *kCLASS_NEVER;
const char *kCLASS_NEVER_DOT;
const char *kMETHOD_NEVER_NC1;
const char *kMETHOD_NEVER_NC2;
const char *kMETHOD_NEVER_NATIVE1;
const char *kMETHOD_NEVER_NATIVE2;
const char *kMETHOD_NEVER_STATIC;

const char *kCLASS_PENDING;
const char *kMETHOD_PENDING_INIT;
const char *kMETHOD_PENDING_INIT_SIG;

const char *kCLASS_RESOLVER;
const char *kMETHOD_RESOLVER_RMA;
const char *kMETHOD_RESOLVER_ENTRYINT;
const char *kMETHOD_RESOLVER_ENTRYCOM;
const char *kFIELD_RESOLVER_DEX;

const char *kCLASS_MSIZE;
const char *kCLASS_MSIZE_DOT;
const char *kMETHOD_MSIZE_METHOD1;
const char *kMETHOD_MSIZE_METHOD2;

const char *kCLASS_CONFIG;
const char *kFIELD_CONFIG_COMPILER;

void *(*ArtClassLinkerFindClass)(void *, char const *, void *) = nullptr;

jint __Get_SDK_INT__(JNIEnv *env) {
    jclass jcBUILD_VERSION = env->FindClass("android/os/Build$VERSION");
    jfieldID jfSDK_INT = env->GetStaticFieldID(jcBUILD_VERSION, "SDK_INT", "I");
    jint SDK_INT = env->GetStaticIntField(jcBUILD_VERSION, jfSDK_INT);
    return SDK_INT;
}

const char *__Get_ART_SO_PATH__(JNIEnv *env) {
    const char *art_lib_path = __Get_SDK_INT__(env) >= 29 ? "/lib64/libart.so" : "/system/lib64/libart.so";
    return art_lib_path;
}

jstring __Get_Class_Name__(JNIEnv *env, jclass clazz) {
    jclass cls = env->FindClass("java/lang/Class");
    jmethodID mid_getName = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    jstring name = (jstring) env->CallObjectMethod(clazz, mid_getName);
    return name;
}

jclass __FindClassEx__(JNIEnv *env, const char *className) {
    jclass jcApplication = env->FindClass("android/app/Application");
    jclass jcVMClassLoader = env->FindClass("java/lang/VMClassLoader");
    jclass jcActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID jmCurrentActivityThread = env->GetStaticMethodID(jcActivityThread,
                                                               "currentActivityThread",
                                                               "()Landroid/app/ActivityThread;");
    jobject joCurrentActivityThread = env->CallStaticObjectMethod(jcActivityThread, jmCurrentActivityThread);
    jmethodID jmGetApplication = env->GetMethodID(jcActivityThread,
                                                  "getApplication", "()Landroid/app/Application;");
    jobject joApplication = env->CallObjectMethod(joCurrentActivityThread, jmGetApplication);
    jmethodID jmGetClassLoader = env->GetMethodID(jcApplication,
                                                  "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject joClassLoader = env->CallObjectMethod(joApplication, jmGetClassLoader);

    jmethodID jmFindLoadedClass = env->GetStaticMethodID(jcVMClassLoader,
                                                         "findLoadedClass",
                                                         "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");

    jstring clazzName = env->NewStringUTF(className);
    jclass clazzInJava = (jclass) env->CallStaticObjectMethod(
            jcVMClassLoader, jmFindLoadedClass, joClassLoader, clazzName);
//    LOGD("----------clazzInJava %s: %p", className, clazzInJava);
//    jclass clazzInEnv = env->FindClass(className);
//    LOGD("----------clazzInJava %s: %p", className, clazzInEnv);

    jclass clazz = clazzInJava;
    return clazz;
}

static const char *
__get_characters_in__(JNIEnv *env, jobjectArray classes_methods_fields, int index) {
    jboolean isCopy = true;
    jstring string = (jstring) env->GetObjectArrayElement(classes_methods_fields, index);
    const char *result = env->GetStringUTFChars(string, &isCopy);
    // env->ReleaseStringUTFChars(string, result);      // should do these after used const char *
    return result;
}

void __FillVariables__(JNIEnv *env, jobject class_loader, jobjectArray classes_methods_fields) {

    kCLASS_SAND = __get_characters_in__(env, classes_methods_fields, 0);
    kMETHOD_SAND_GETART = __get_characters_in__(env, classes_methods_fields, 1);
    kMETHOD_SAND_GETART_SIG = __get_characters_in__(env, classes_methods_fields, 2);
    kMETHOD_SAND_GETJAVA = __get_characters_in__(env, classes_methods_fields, 3);
    kMETHOD_SAND_GETJAVA_SIG = __get_characters_in__(env, classes_methods_fields, 4);
    kMETHOD_SAND_GETTHREAD = __get_characters_in__(env, classes_methods_fields, 5);
    kFIELD_SAND_ACCESS = __get_characters_in__(env, classes_methods_fields, 6);

    kCLASS_NEVER = __get_characters_in__(env, classes_methods_fields, 7);
    kCLASS_NEVER_DOT = __get_characters_in__(env, classes_methods_fields, 8);
    kMETHOD_NEVER_NC1 = __get_characters_in__(env, classes_methods_fields, 9);
    kMETHOD_NEVER_NC2 = __get_characters_in__(env, classes_methods_fields, 10);
    kMETHOD_NEVER_NATIVE1 = __get_characters_in__(env, classes_methods_fields, 11);
    kMETHOD_NEVER_NATIVE2 = __get_characters_in__(env, classes_methods_fields, 12);
    kMETHOD_NEVER_STATIC = __get_characters_in__(env, classes_methods_fields, 13);

    kCLASS_PENDING = __get_characters_in__(env, classes_methods_fields, 14);
    kMETHOD_PENDING_INIT = __get_characters_in__(env, classes_methods_fields, 15);
    kMETHOD_PENDING_INIT_SIG = __get_characters_in__(env, classes_methods_fields, 16);

    kCLASS_RESOLVER = __get_characters_in__(env, classes_methods_fields, 17);
    kMETHOD_RESOLVER_RMA = __get_characters_in__(env, classes_methods_fields, 18);
    kMETHOD_RESOLVER_ENTRYINT = __get_characters_in__(env, classes_methods_fields, 19);
    kMETHOD_RESOLVER_ENTRYCOM = __get_characters_in__(env, classes_methods_fields, 20);
    kFIELD_RESOLVER_DEX = __get_characters_in__(env, classes_methods_fields, 21);

    kCLASS_MSIZE = __get_characters_in__(env, classes_methods_fields, 22);
    kCLASS_MSIZE_DOT = __get_characters_in__(env, classes_methods_fields, 23);
    kMETHOD_MSIZE_METHOD1 = __get_characters_in__(env, classes_methods_fields, 24);
    kMETHOD_MSIZE_METHOD2 = __get_characters_in__(env, classes_methods_fields, 25);

    kCLASS_CONFIG = __get_characters_in__(env, classes_methods_fields, 26);
    kFIELD_CONFIG_COMPILER = __get_characters_in__(env, classes_methods_fields, 27);


    ArtClassLinkerFindClass = reinterpret_cast<void *(*)(void *, char const *, void *)>(
            getSymCompat(__Get_ART_SO_PATH__(env),
                         "_ZN3art11ClassLinker9FindClassEPNS_6ThreadEPKcNS_6HandleINS_6mirror11ClassLoaderEEE")
    );

}