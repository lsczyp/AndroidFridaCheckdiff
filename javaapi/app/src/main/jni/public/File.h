//
// Created by lishengchang1 on 2016/7/14.
//

#ifndef SO_PROJECT_FILE_H
#define SO_PROJECT_FILE_H
#include <jni.h>

extern "C" {
size_t file_size(char *filename);

bool writefile(char *path, char *buf, int len);

char *readfile(char *path, int &nretlen);

bool fileexists(char *path);

void createfile(char *path);

void deletefile(char *path);

char *jstringTostring(JNIEnv *env, jstring jstr);

jstring strToJstring(JNIEnv *env, const char *pStr);

jstring jstringconcat(JNIEnv *env, jstring jstr1, const char *concat);

bool fileexistsbyjava(JNIEnv *env, char *s);

void createfilebyjava(JNIEnv *env, char *s);

void deletefilebyjava(JNIEnv *env, char *s);

void Writefilebyjava(JNIEnv *env, char *s, char *buf, int len);

void *readfilebymmap(char *path, int length, int &nPageSize);

void freefilebymumap(void *pv, int nPageSize);

}
#endif //SO_PROJECT_FILE_H
