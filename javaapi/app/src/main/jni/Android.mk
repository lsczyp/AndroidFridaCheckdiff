
LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/public \
                    $(JNI_H_INCLUDE)

LOCAL_MODULE    := elf

LOCAL_SRC_FILES := JNI_OnLoad.cpp Interface.cpp\
                public/elfinfo.cpp public/File.cpp\


LOCAL_LDLIBS := -llog -landroid
LOCAL_CFLAGS += -fvisibility-ms-compat
include $(BUILD_SHARED_LIBRARY)