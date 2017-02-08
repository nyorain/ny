LOCAL_PATH := $(call my-dir)

# ny
include $(CLEAR_VARS)
LOCAL_MODULE := ny
LOCAL_SRC_FILES := libny.so
include $(PREBUILT_SHARED_LIBRARY)

# android example
include $(CLEAR_VARS)

LOCAL_MODULE    := ny-android
LOCAL_SRC_FILES := main.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)//../../../src
LOCAL_C_INCLUDES += $(LOCAL_PATH)//../../../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)//../external/install/include
LOCAL_CPPFLAGS += -std=c++1z
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_SHARED_LIBRARIES := ny

include $(BUILD_SHARED_LIBRARY)
