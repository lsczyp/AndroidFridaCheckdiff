//
// Created by lishe on 2019/9/24.
//

#ifndef MOBILEPROTECT_INTERFACE_H
#define MOBILEPROTECT_INTERFACE_H


#include <jni.h>

extern "C" {
jlong JNICALL mapload(JNIEnv * env , jobject thiz,jstring libname);
jlong JNICALL getImport(JNIEnv *env, jobject thiz,jlong base, jstring funcname);
jlong JNICALL getExport(JNIEnv *env, jobject thiz,jlong base, jstring funcname);
jlong JNICALL getCode(JNIEnv *env, jobject thiz,jlong base, jstring funcname);
}

#endif //MOBILEPROTECT_INTERFACE_H
