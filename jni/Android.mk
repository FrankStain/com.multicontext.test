# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE			:= nt-core
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/include $(LOCAL_PATH)/source

LOCAL_CPPFLAGS			:=			\
	-DANDROID_NDK					\
	-D__ANDROID__					\
	-D_REENTRANT					\
	-D_STLP_USE_NEWALLOC			\
	-D_THREAD_SAFE					\
	-D_POSIX_THREADS				\
	-DENABLE_ACTIVITY_LOGS			\
	-DENABLE_THREAD_LOGS			\
	-DUSE_GRAPHICS_LOGS				\
	-DUSE_GRAPHICS_BUFFERS_LOGS		\

# Build switchers :
#
#	Enable second (storage) context and use it in separate thread to prepare VBO && IBO
#	-DUSE_MULTICONTEXT				\
#
#	Use selected IBO instead of IBO direct memory (DMA) pointer
#	-DUSE_SELECT_IBO				\

LOCAL_SRC_FILES			:=			\
	source/cIndexBuffer.cpp			\
	source/cVertexBuffer.cpp		\
	source/cConfigurator.cpp		\
	source/cGraphics.cpp			\
	source/cNativeApplication.cpp	\
	source/cNativeThread.cpp		\
	source/main.cpp					\

LOCAL_LDLIBS			:= -llog -landroid -lEGL -lGLESv1_CM
LOCAL_STATIC_LIBRARIES	:= android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
