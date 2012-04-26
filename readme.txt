Light android application which demonstrate how can we work with NativeActivity and OpenGL 1.1.
The subject of this example include two problems, that presents in Android OpenGL/ES development:
	1- EGL contexts with sharegroups. If you need to create two (or more... but why more? 0_o) connected (shared) EGL context, on Adreno GPU's it will causes serious problems like SIGSEGV.
	2- glDrawElements() with binded IBO(GL_ELEMENT_ARRAY_BUFFER). With SGX GPU's it will works fine, but on Adreno (tested on 205 && 220) GPU's using glDrawElements() with binded IBO and non-NULL 'inbices' argument causes SIGSEGV in 'libGLESv1_CM_adreno200.so'.
	
So, example demonstrates this problems.
When problem will be solved, i will make a commit to show the problem solution.

Building tested on NDK r7b and SDK 19. Just use 'ndk-build NDK_DEBUG=1' and make clean the eclipse project after build.
By default project will be built with single EGL context, with single-thread loading and with using DMA 'indices' in glDrawElements().
If you want to use IBO selection instead DMA pointers, you need to include 'USE_SELECT_IBO' macro to the 'LOCAL_CPPFLAGS' in 'Android.mk'.
If you want to use multiple EGL contexts and data loading in a separate thread (which use second context), you need to include 'USE_MULTICONTEXT' macro to the 'LOCAL_CPPFLAGS' in 'Android.mk'.

Links to issues, when you can discuss problems and example :
(glDrawElements() cause SIGSEGV when used with IBO and offset 'indices' pointer on Adreno GPU's)
https://groups.google.com/d/topic/android-platform/Az1Wb3vK1N8/discussion

(EGL context with sharegroups)
https://groups.google.com/d/topic/android-developers/qTFKOY46Swc/discussion

(Index Buffer Object offset causing crash with OpenGL ES 1.1)
https://developer.qualcomm.com/forum/qdevnet-forums/mobile-gaming-graphics-optimization-adreno/8883