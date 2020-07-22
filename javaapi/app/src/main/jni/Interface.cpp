//
// Created by lishe on 2019/9/24.
//

#include "Interface.h"
#include "public/File.h"
#include "public/elfinfo.h"

extern "C" {
jlong JNICALL mapload(JNIEnv * env , jobject thiz,jstring libname) {
    int nRet = -1;
    int nPageSize = 0;
    void * base = NULL;
    char *s = jstringTostring(env, libname);
    if(s){
        int size = file_size(s);
        if(size <= 0){
            ;
        } else{
            base = readfilebymmap(s,size,nPageSize);
        }
    }
    return (jlong)base;
}

jlong JNICALL getImport(JNIEnv *env, jobject thiz,jlong base, jstring funcname){
    char *s = jstringTostring(env, funcname);
    if(s){
        return (jlong)getapi_addr_REL((void*)base,s, IN_FILE);
    }
    return 0;
}

jlong JNICALL getExport(JNIEnv *env, jobject thiz,jlong base, jstring funcname){
    jlong ex = 0;
    char *s = jstringTostring(env, funcname);
    if(s){
        API_uAddr_pAddr aup = {0};
        GetsoAPIaddr((void*)base,s, IN_FILE,aup);
        if(aup.uAddr == 0){
            ;
        } else{
            ex = (jlong)aup.pAddr;
        }
    }
    return ex;
}

jlong JNICALL getCode(JNIEnv *env, jobject thiz,jlong base, jstring funcname){
    jlong ex = 0;
    char *s = jstringTostring(env, funcname);
    if(s){
        API_uAddr_pAddr aup = {0};
        GetsoAPIaddr((void*)base,s, IN_FILE,aup);
        if(aup.uAddr == 0){
            ;
        } else{
            ex = (jlong)aup.uAddr;
        }
    }
    return ex;
}

}