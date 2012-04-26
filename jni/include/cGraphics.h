#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <pthread.h>
#include <android/native_window.h>
#include "cNativeMutex.h"
#include "logs.h"

class cGraphics
{
private:
	
	struct cEGLContextData {
		bool			m_is_setup;
		bool			m_is_current;
		EGLConfig		m_config;
		EGLContext		m_context;
		EGLSurface		m_surface;

		cEGLContextData() : m_is_setup(true), m_is_current(false), m_config(NULL), m_context(EGL_NO_CONTEXT), m_surface(EGL_NO_SURFACE) {};
	};
	
	static cGraphics*	m_instance;

	pthread_t			m_host_thread;
	pthread_t			m_lock_thread;
	EGLDisplay			m_display;
	cEGLContextData		m_render;
	cEGLContextData		m_storage;
	EGLint				m_version[2];

#ifdef USE_MULTICONTEXT
	cCritSection		m_context_lock;
#endif

	void setup_context();

protected:
	cGraphics();

public:
	static cGraphics* instance() { return ( m_instance )? m_instance : new cGraphics(); };
	virtual ~cGraphics();

	const bool init();
	void final();

	const bool set_window( ANativeWindow* );
	const bool unset_window();

	static void lock();
	static void unlock();

	static const bool is_ready() { return ( m_instance )? m_instance->m_render.m_is_current : false; };

	void swap();
};