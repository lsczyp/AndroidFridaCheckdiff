/*
 * log.h
 *
 *  Created on: 2015-11-2
 *      Author: lishengchang
 */

#ifndef LOG_H_
#define LOG_H_
#include <android/log.h>


#define __LOGOFF__

#define _TAG_LOG "ELF"

#ifdef __LOGOFF__
#define LOGE(...)
#define LOGD(...)
#define LOGW(...)
#define LOGI(...)
#else
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, _TAG_LOG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, _TAG_LOG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, _TAG_LOG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, _TAG_LOG, __VA_ARGS__)
#endif

//这条log是强制输出，只为观察问题用
#define LOGQZSC(...) __android_log_print(ANDROID_LOG_ERROR, _TAG_LOG, __VA_ARGS__)

#endif /* LOG_H_ */
