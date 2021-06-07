//
// Created by ubuntu on 5/28/21.
//

#ifndef SANDLOCK_JAVA_BRIDGE_H
#define SANDLOCK_JAVA_BRIDGE_H

#include <jni.h>
#include <stdlib.h>
#include <string.h>

extern const char *kCLASS_SAND;
extern const char *kMETHOD_SAND_GETART;
extern const char *kMETHOD_SAND_GETART_SIG;
extern const char *kMETHOD_SAND_GETJAVA;
extern const char *kMETHOD_SAND_GETJAVA_SIG;
extern const char *kMETHOD_SAND_GETTHREAD;
extern const char *kFIELD_SAND_ACCESS;

extern const char *kCLASS_NEVER;
extern const char *kCLASS_NEVER_DOT;
extern const char *kMETHOD_NEVER_NC1;
extern const char *kMETHOD_NEVER_NC2;
extern const char *kMETHOD_NEVER_NATIVE1;
extern const char *kMETHOD_NEVER_NATIVE2;
extern const char *kMETHOD_NEVER_STATIC;

extern const char *kCLASS_PENDING;
extern const char *kMETHOD_PENDING_INIT;
extern const char *kMETHOD_PENDING_INIT_SIG;

extern const char *kCLASS_RESOLVER;
extern const char *kMETHOD_RESOLVER_RMA;
extern const char *kMETHOD_RESOLVER_ENTRYINT;
extern const char *kMETHOD_RESOLVER_ENTRYCOM;
extern const char *kFIELD_RESOLVER_DEX;

extern const char *kCLASS_MSIZE;
extern const char *kCLASS_MSIZE_DOT;
extern const char *kMETHOD_MSIZE_METHOD1;
extern const char *kMETHOD_MSIZE_METHOD2;

extern const char *kCLASS_CONFIG;
extern const char *kFIELD_CONFIG_COMPILER;

extern void *(*ArtClassLinkerFindClass)(void *, char const *, void *);

jint __Get_SDK_INT__(JNIEnv *env);

const char *__Get_ART_SO_PATH__(JNIEnv *env);

jstring __Get_Class_Name__(JNIEnv *env, jclass clazz);

jclass __FindClassEx__(JNIEnv *env, const char *className);

void __FillVariables__(JNIEnv *env, jobject class_loader, jobjectArray classes_methods_fields);


#endif //SANDLOCK_JAVA_BRIDGE_H
