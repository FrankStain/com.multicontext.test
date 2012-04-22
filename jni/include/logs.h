#pragma once

#include <jni.h>
#include <errno.h>
#include <android/log.h>
#include <algorithm>
#include <string>

extern char LOG_TAG[];

static void android_trace_log( const int type, const char* func, const int line, const char* msg, ... )
{
	static char format[ 1025 ] = { 0 };

	va_list argptr;
	va_start( argptr, msg );

#ifdef FREE_LOG_BUFFER
	memset( format, 0, 1025 );
#endif
	snprintf( format, 1024, "%s [%i] : %s", func, line, msg );
	__android_log_vprint( type, LOG_TAG, format, argptr );

	va_end( argptr );
};

#define NT_LOG_VERB(...)	android_trace_log( ANDROID_LOG_VERBOSE, __func__, __LINE__, __VA_ARGS__ )
#define NT_LOG_INFO(...)	android_trace_log( ANDROID_LOG_INFO, __func__, __LINE__, __VA_ARGS__ )
#define NT_LOG_WARN(...)	android_trace_log( ANDROID_LOG_WARN, __func__, __LINE__, __VA_ARGS__ )
#define NT_LOG_DBG(...)		android_trace_log( ANDROID_LOG_DEBUG, __func__, __LINE__, __VA_ARGS__ )
#define NT_LOG_ERR(...)		android_trace_log( ANDROID_LOG_ERROR, __func__, __LINE__, __VA_ARGS__ )