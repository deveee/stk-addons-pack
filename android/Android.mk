LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Freetype
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := obj/freetype/objs/.libs/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

# zlib
LOCAL_MODULE := zlib
LOCAL_SRC_FILES := obj/zlib/libz.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

# PNG
LOCAL_MODULE := png
LOCAL_SRC_FILES := obj/libpng/libpng.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

# Turbojpeg
LOCAL_MODULE := turbojpeg
LOCAL_SRC_FILES := obj/libjpeg-turbo/.libs/libturbojpeg.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

# Main
LOCAL_MODULE       := main
LOCAL_PATH         := .
LOCAL_CPP_FEATURES += rtti exceptions
LOCAL_SRC_FILES    := $(wildcard ../src/*.cpp)     \
                      $(wildcard ../src/*/*.cpp)   \
                      $(wildcard ../src/*/*/*.cpp)
LOCAL_LDLIBS       := -llog -landroid -lEGL -lGLESv3 -lOpenSLES
LOCAL_CFLAGS       := -I../src               \
                      -Iobj/freetype/include \
                      -Iobj/libpng/          \
                      -Iobj/zlib/            \
                      -Iobj/libjpeg-turbo    \
                      -I$(call my-dir)/../../sources/android/native_app_glue \
                      -DNDEBUG               \
                      -std=gnu++0x

LOCAL_STATIC_LIBRARIES := freetype png zlib turbojpeg gnustl_static \
                          android_native_app_glue

include $(BUILD_SHARED_LIBRARY)
include $(CLEAR_VARS)

$(call import-module,android/native_app_glue)
