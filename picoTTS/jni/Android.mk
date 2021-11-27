# SVOX Pico TTS Engine
# This makefile builds both an activity and a shared library.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := PicoTTS
	
LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-java-files-under, compat)

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_LDLIBS += -lutils -lcutils -llog -ldl

include $(BUILD_PACKAGE)

include  $(LOCAL_PATH)/../libExpat/trunk/jni/Android.mk \
    $(LOCAL_PATH)/../picoEngine/jni/Android.mk \
    $(LOCAL_PATH)/../picoEngine_AndroidWrapper/jni/Android.mk
