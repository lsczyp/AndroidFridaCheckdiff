#include <jni.h>
#include <unistd.h>
#include "Interface.h"
#include "public/xylog.h"


extern "C" {

static JNINativeMethod methods[] = {
		{"mapload", "(Ljava/lang/String;)J", (void*)mapload},
		{"getImport", "(JLjava/lang/String;)J", (void*)getImport},
        {"getExport", "(JLjava/lang/String;)J", (void*)getExport},
		{"getCode", "(JLjava/lang/String;)J", (void*)getCode}
};


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	int nRet = JNI_ERR;
	LOGD("JNI_OnLoad in");

	JNIEnv *env = NULL;

	if(vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK){
		return nRet;
	}

	if (env == NULL) {
		return nRet;
	}


//以下为JNI导出函数的绑定过程

	int nMethodsNum = sizeof(methods) / sizeof(methods[0]);

	jclass clazz = env->FindClass("com/jlxy/javaapi/C");
	if (clazz == NULL) {
		return nRet;
	}

	if (env->RegisterNatives(clazz, methods, nMethodsNum)  != JNI_OK) {
		return nRet;
	}

	LOGD("JNI_OnLoad out");
	return JNI_VERSION_1_6;
}

}




