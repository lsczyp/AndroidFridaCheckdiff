//
// Created by lishengchang1 on 2016/7/14.
//

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "File.h"
#include "xylog.h"
#include "sys/stat.h"

extern "C" {
#define SAFE_FREE(x) if(x){free(x);x=NULL;}

size_t file_size(char *filename) {
    int fd = open(filename, O_RDONLY, 0);
    if (fd== -1){
        return (size_t)-1;
    }
    struct stat sbuf = {0};
    memset(&sbuf, 0, sizeof(sbuf));
    if (fstat(fd, &sbuf) == -1) {
        LOGE("%s,error:%d,%s",filename,errno,strerror(errno));
        close(fd);
        return (size_t)-2;
    }
    close(fd);
    return (size_t)sbuf.st_size;
}


bool writefile(char *path, char *buf, int len) {
    bool b = false;
    FILE *fp = fopen(path, "wb");
    if (fp) {
        int n = fwrite(buf, len, 1, fp);
        if (n != 1) {
            LOGE("%s writefile failed! %d, %d,%s", path, n, errno, strerror(errno));
            /*if(errno == ENOSPC)*/{

            }
        } else {
            b = true;
        }
        fclose(fp);
    }
    return b;
}

char *readfile(char *path, int &nretlen) {
    char *buf = NULL;
    size_t len = file_size(path);
    FILE *fp = fopen(path, "rb");
    if (fp) {
        nretlen = len;
        buf = (char *) malloc(len);
        if(buf){
            size_t  rl = fread(buf, len, 1, fp);
            if(rl != 1)
                SAFE_FREE(buf);
        }

        fclose(fp);
    }
    return buf;
}

bool fileexists(char *path) {
    bool b = false;
    if ((access(path, F_OK)) == 0) {
        LOGD("linux c %s exists", path);
        b = true;
    } else {
        LOGD("linux c %s not exists", path);
        b = false;
    }
    return b;
}

void createfile(char *path) {
    int fd = 0;
    fd = open(path, O_CREAT, S_IRUSR|S_IWUSR);
    if (fd)
        close(fd);
}

void deletefile(char *path){
    remove(path);
}

char *j_jstringTostring(JNIEnv *env, jstring jstr) {
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);

        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

char *jstringTostring(JNIEnv *env, jstring jstr) {
    jboolean tureforGTC = 0;
    char *rtn = (char *) env->GetStringUTFChars(jstr, &tureforGTC);
    if (rtn == NULL)
        rtn = j_jstringTostring(env, jstr);
    return rtn;
}

//C字符串转java字符串
jstring strToJstring(JNIEnv *env, const char *pStr) {
    int strLen = strlen(pStr);
    jclass jstrObj = env->FindClass("java/lang/String");
    jmethodID methodId = env->GetMethodID(jstrObj, "<init>", "([BLjava/lang/String;)V");
    jbyteArray byteArray = env->NewByteArray(strLen);
    jstring encode = env->NewStringUTF("utf-8");

    env->SetByteArrayRegion(byteArray, 0, strLen, (jbyte *) pStr);

    jstring jsret = (jstring) env->NewObject(jstrObj, methodId, byteArray, encode);

    env->DeleteLocalRef(byteArray);
    env->DeleteLocalRef(encode);

    return jsret;
}

jstring jstringconcat(JNIEnv *env, jstring jstr1, const char *concat) {
    jclass jstrObj = env->FindClass("java/lang/String");
    jmethodID methodId = env->GetMethodID(jstrObj, "concat",
                                          "(Ljava/lang/String;)Ljava/lang/String;");
    jstring jsconcat = env->NewStringUTF(concat);
    jstring jsret = (jstring) env->CallObjectMethod(jstr1, methodId, jsconcat);
    env->DeleteLocalRef(jsconcat);
    return jsret;
}


bool fileexistsbyjava(JNIEnv *env, char *s) {
    bool b = false;
    jstring js = strToJstring(env, s);
    jclass cls_File = env->FindClass("java/io/File");
    jmethodID mid_File_new = env->GetMethodID(cls_File, "<init>", "(Ljava/lang/String;)V");
    jobject obj_fileDexPath = env->NewObject(cls_File, mid_File_new, js);

    jmethodID mid_File_exists = env->GetMethodID(cls_File, "exists", "()Z");
    if (!(bool) env->CallBooleanMethod(obj_fileDexPath, mid_File_exists)) {
        LOGD("%s not exists", s);
        b = false;
    }
    else {
        LOGD("%s exists", s);
        b = true;
    }
    return b;
}

void createfilebyjava(JNIEnv *env, char *s) {
    jstring js = strToJstring(env, s);
    jclass cls_File = env->FindClass("java/io/File");
    jmethodID mid_File_new = env->GetMethodID(cls_File, "<init>", "(Ljava/lang/String;)V");
    jobject obj_fileDexPath = env->NewObject(cls_File, mid_File_new, js);

    LOGD("%s createNewFile", s);
    jmethodID mid_File_createNewFile = env->GetMethodID(cls_File, "createNewFile", "()Z");
    env->CallBooleanMethod(obj_fileDexPath, mid_File_createNewFile);
}

void deletefilebyjava(JNIEnv *env, char *s) {
    jstring js = strToJstring(env, s);
    jclass cls_File = env->FindClass("java/io/File");
    jmethodID mid_File_new = env->GetMethodID(cls_File, "<init>", "(Ljava/lang/String;)V");
    jobject obj_filePath = env->NewObject(cls_File, mid_File_new, js);

    jmethodID mid_File_delete = env->GetMethodID(cls_File, "delete", "()Z");
    env->CallBooleanMethod(obj_filePath, mid_File_delete);
}

void Writefilebyjava(JNIEnv *env, char *s, char *buf, int len) {
    jstring js = strToJstring(env, s);
    jclass cls_FileOutputStream = env->FindClass("java/io/FileOutputStream");
    jmethodID mid_cls_FileOutputStream_new = env->GetMethodID(cls_FileOutputStream, "<init>",
                                                              "(Ljava/lang/String;)V");
    jobject obj_filePath = env->NewObject(cls_FileOutputStream, mid_cls_FileOutputStream_new, js);

    jmethodID mid_File_Write = env->GetMethodID(cls_FileOutputStream, "write", "([BII)V");

    jbyte *by = (jbyte *) buf;
    jbyteArray jarray = env->NewByteArray(len);
    env->SetByteArrayRegion(jarray, 0, len, by);

    jint ji = len;
    env->CallVoidMethod(obj_filePath, mid_File_Write, jarray, 0, ji);

    env->DeleteLocalRef(jarray);

    jmethodID mid_File_close = env->GetMethodID(cls_FileOutputStream, "close", "()V");
    env->CallVoidMethod(obj_filePath, mid_File_close);

}

void *readfilebymmap(char *path, int length, int &nPageSize) {

    void *pv = NULL;
    int fd = open(path, O_RDONLY, 0);
    if (fd) {
        int PageSize = (length + 0x1000) & 0xFFFFF000;
        nPageSize = PageSize;
        pv = mmap(NULL, PageSize, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
    }
    return pv;
}

void freefilebymumap(void *pv, int nPageSize) {
    int n = munmap(pv, nPageSize);
    if (n == -1) {
        LOGE("munmap %p failed %d %s!", pv, errno, strerror(errno));
    } else {
        LOGD("munmap %p ok!", pv);
    }
}

}