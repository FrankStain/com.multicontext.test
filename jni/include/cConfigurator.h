#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <list>
#include "logs.h"

class cEGLConfig {

	friend class cEGLConfigList;

private:
	EGLDisplay*	m_display;
	EGLConfig*	m_config;

	int32_t get_option( EGLint ) const;

public:
	cEGLConfig() : m_display(NULL), m_config(NULL) {};
	cEGLConfig( const cEGLConfig& p ) : m_display( p.m_display ), m_config( p.m_config ) {};
	cEGLConfig( EGLDisplay* disp, EGLConfig* conf ) : m_display( disp ), m_config( conf ) {};
	virtual ~cEGLConfig() { m_display = NULL; m_config = NULL; };
	
	const EGLDisplay* display() const { return m_display; };
	const EGLConfig* config() const { return m_config; };

	const int32_t id()					const { return get_option( EGL_CONFIG_ID ); };
	const int32_t caveat()				const { return get_option( EGL_CONFIG_CAVEAT ); };
	const int32_t conformat()			const { return get_option( EGL_CONFORMANT ); };
	
	const int32_t max_pbuffer_width()	const { return get_option( EGL_MAX_PBUFFER_WIDTH ); };
	const int32_t max_pbuffer_height()	const { return get_option( EGL_MAX_PBUFFER_HEIGHT ); };

	const int32_t depth()				const { return get_option( EGL_DEPTH_SIZE ); };
	const int32_t stencil()				const { return get_option( EGL_STENCIL_SIZE ); };
	const int32_t red()					const { return get_option( EGL_RED_SIZE ); };
	const int32_t green()				const { return get_option( EGL_GREEN_SIZE ); };
	const int32_t blue()				const { return get_option( EGL_BLUE_SIZE ); };
	const int32_t alpha()				const { return get_option( EGL_ALPHA_SIZE ); };
	const int32_t buffer_size()			const { return get_option( EGL_BUFFER_SIZE ); };
	
	const int32_t samples()				const { return get_option( EGL_SAMPLES ); };
	const int32_t sample_buffers()		const { return get_option( EGL_SAMPLE_BUFFERS ); };
	const int32_t surface_type()		const { return get_option( EGL_SURFACE_TYPE ); };
	const int32_t render_type()			const { return get_option( EGL_RENDERABLE_TYPE ); };

	const int32_t native_visual_type()	const { return get_option( EGL_NATIVE_VISUAL_TYPE ); };
	const int32_t native_visual_id()	const { return get_option( EGL_NATIVE_VISUAL_ID ); };
	const bool native_renderable()		const { return get_option( EGL_NATIVE_RENDERABLE ); };

	void print() const;
	
	void operator = ( const cEGLConfig& conf ) { m_display = conf.m_display; m_config = conf.m_config; };
};

class cEGLConfigList {
private:
	const EGLDisplay*		m_display;
	EGLConfig*				m_configs;
	int32_t					m_count;

	std::list<cEGLConfig>	m_list;
	cEGLConfig				m_dummy;
	
public:
	cEGLConfigList( const EGLDisplay* );
	virtual ~cEGLConfigList();

	void filter_flush();
	const int32_t count() const { return m_list.size(); };
	const int32_t total_configs() const { return m_count; };

	const int32_t filter_in_buffer_size( const int32_t );
	const int32_t filter_in_rgb( const int32_t, const int32_t, const int32_t );
	const int32_t filter_in_rgba( const int32_t, const int32_t, const int32_t, const int32_t );
	const int32_t filter_in_samples( const int32_t );
	const int32_t filter_in_native_visual_id( const int32_t );
	const int32_t filter_out_native_visual_id( const int32_t );
	const int32_t filter_in_gl_version( const int32_t );
	const int32_t filter_out_gl_version( const int32_t );
	const int32_t filter_in_surface_type( const int32_t );
	const int32_t filter_out_surface_type( const int32_t );

	void sort_by_depth( const bool = false );
	void sort_by_stencil( const bool = false );
	void sort_by_samples( const bool = false );
	void sort_by_caveat();

	void print();

	const cEGLConfig& operator []( int32_t index ){
		if( ( index > m_list.size() ) || ( 0 > index ) ){
			return m_dummy;
		};

		std::list<cEGLConfig>::iterator c = m_list.begin();
		while( index-- ){
			c++;
		};

		return (*c);
	};
};

typedef cEGLConfigList cConfigurator;