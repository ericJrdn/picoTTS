LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := picoEngine_androidWrapper
	
LOCAL_SRC_FILES := \
	cutils_strdup8to16.cpp \
	cutils_strdup16to8.cpp \
    com_android_tts_compat_SynthProxy.cpp \
	picottsengine.cpp \
	svox_ssml_parser.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_LDLIBS += -llog -ldl -lstdc++

LOCAL_STATIC_LIBRARIES := picoEngine
LOCAL_SHARED_LIBRARIES := libexpat

include $(BUILD_SHARED_LIBRARY)




