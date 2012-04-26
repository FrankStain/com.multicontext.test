#pragma once

#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "cNativeApplication.h"
#include "cGraphics.h"
#include "logs.h"

class cMainCommandProcessor : public cCommandProcessor {
public:
	virtual const bool process_command( const sAppMessage&, cNativeApplication& );
};
