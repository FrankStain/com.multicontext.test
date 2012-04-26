#include "cConfigurator.h"
#include <algorithm>

using namespace std;

#ifdef USE_GRAPHICS_LOGS
#define G_LOG_I			NT_LOG_INFO
#define G_LOG_W			NT_LOG_WARN
#define G_LOG_D			NT_LOG_DBG
#else
#define G_LOG_I(...)
#define G_LOG_W(...)
#define G_LOG_D(...)
#endif

#define	G_LOG_E			NT_LOG_ERR
#define G_ENTER(...)	G_LOG_D( "ENTER" )
#define G_EXIT(...)		G_LOG_D( "EXIT" )
#define G_FAIL(...)		G_LOG_W( "EXIT" )

int32_t cEGLConfig::get_option( EGLint option ) const {
	EGLint op_result;
	bool res = ( m_display && m_config )? eglGetConfigAttrib( *m_display, *m_config, option, &op_result ) : false;
	return ( res )? op_result : EGL_FALSE;
};

void cEGLConfig::print() const {
	G_LOG_D( "----------------------------------------" );
	G_LOG_D(
		"[ EGL_CONFIG_ID %02d; EGL_RENDERABLE_TYPE 0x%04X; EGL_NATIVE_RENDERABLE 0x%02X; EGL_NATIVE_VISUAL_ID 0x%04X; EGL_NATIVE_VISUAL_TYPE 0x%04X ]",
		id(), render_type(), native_renderable(), native_visual_id(), native_visual_type()
	);
	G_LOG_D(
		"[ EGL_BUFFER_SIZE %d; EGL_RED_SIZE %d; EGL_GREEN_SIZE %d; EGL_BLUE_SIZE %d; EGL_ALPHA_SIZE %d; EGL_DEPTH_SIZE %d; EGL_STENCIL_SIZE %d ]",
		buffer_size(), red(), green(), blue(), alpha(), depth(), stencil()
	);
	G_LOG_D(
		"[ EGL_SURFACE_TYPE 0x%04X; EGL_LEVEL %d; EGL_CONFIG_CAVEAT 0x%04X(%d); EGL_SAMPLES %d; EGL_SAMPLE_BUFFERS %d; EGL_CONFORMANT 0x%04X(%d) ]",
		surface_type(), get_option( EGL_LEVEL ), get_option( EGL_CONFIG_CAVEAT ), get_option( EGL_CONFIG_CAVEAT ),
		samples(), sample_buffers(), get_option( EGL_CONFORMANT ), get_option( EGL_CONFORMANT )
	);
	G_LOG_D( "[ EGL_TEXTURE_FORMAT 0x%04X ]", get_option( EGL_TEXTURE_FORMAT ) );
	G_LOG_D( "----------------------------------------" );
};

cEGLConfigList::cEGLConfigList( const EGLDisplay* display ) : m_display(display), m_configs(NULL), m_count(0) {
	G_ENTER();
	if( !m_display ){
		G_LOG_E( "No EGL display is set" );
		return;
	};

	G_LOG_I( "get EGL configs count" );
	if( !( eglGetConfigs( *m_display, NULL, 0, (EGLint*)&m_count ) && m_count ) ){
		G_LOG_E( "eglGetConfigs() failed with code 0x%08X", eglGetError() );
		return;
	};

	m_configs = new EGLConfig[ m_count ];
	int conf_count = 0;

	G_LOG_I( "enum EGL configs" );
	if( !( eglGetConfigs( *m_display, m_configs, m_count, (EGLint*)&conf_count ) && conf_count ) ){
		G_LOG_E( "eglGetConfigs() failed with code 0x%08X", eglGetError() );
		return;
	};

	filter_flush();

	G_EXIT();
};

cEGLConfigList::~cEGLConfigList(){
	G_ENTER();
	m_list.clear();
	delete[] m_configs;
	m_configs	= NULL;
	m_count		= 0;
	G_EXIT();
};

void cEGLConfigList::print(){
	G_ENTER();
	G_LOG_I( "filtered configs count : %d", m_list.size() );
	for( list<cEGLConfig>::iterator c = m_list.begin(); m_list.end() != c; c++ ){
		c->print();
	};
	G_EXIT();
};

void cEGLConfigList::filter_flush(){
	G_ENTER();
	m_list.clear();
	for( int c = 0; m_count > c; c++ ){
		m_list.push_back( cEGLConfig( (EGLDisplay*)m_display, &m_configs[ c ] ) );
	};
	G_EXIT();
};

const int32_t cEGLConfigList::filter_in_buffer_size( const int32_t size ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( size > conf->buffer_size() ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_rgb( const int32_t red, const int32_t green, const int32_t blue ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( ( red > conf->red() ) || ( green > conf->green() ) || ( blue > conf->blue() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_rgba( const int32_t red, const int32_t green, const int32_t blue, const int32_t alpha ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( ( red > conf->red() ) || ( green > conf->green() ) || ( blue > conf->blue() ) || ( alpha > conf->alpha() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_samples( const int32_t samples ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( samples > conf->samples() ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_native_visual_id( const int32_t id ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( conf->native_visual_id() && ( id != conf->native_visual_id() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_out_native_visual_id( const int32_t id ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( conf->native_visual_id() && ( id == conf->native_visual_id() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_gl_version( const int32_t flags ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( flags != ( flags & conf->render_type() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_out_gl_version( const int32_t flags ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( flags == ( flags & conf->render_type() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_in_surface_type( const int32_t flags ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( flags != ( flags & conf->surface_type() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

const int32_t cEGLConfigList::filter_out_surface_type( const int32_t flags ){
	G_ENTER();
	if( !m_list.size() ){
		G_FAIL();
		return m_list.size();
	};

	list<cEGLConfig>::iterator conf = m_list.begin();
	while( m_list.end() != conf ){
		if( flags == ( flags & conf->surface_type() ) ){
			conf = m_list.erase( conf );
		}else{
			conf++;
		};
	};
	G_EXIT();
	return m_list.size();
};

bool pred_depth_up( const cEGLConfig& x, const cEGLConfig& y ){
	return y.depth() > x.depth();
};

bool pred_depth_down( const cEGLConfig& x, const cEGLConfig& y ){
	return x.depth() > y.depth();
};

void cEGLConfigList::sort_by_depth( const bool up_order ){
	G_ENTER();
	m_list.sort( ( up_order )? pred_depth_up : pred_depth_down );
	G_EXIT();
};

bool pred_stencil_up( const cEGLConfig& x, const cEGLConfig& y ){
	return y.stencil() > x.stencil();
};

bool pred_stencil_down( const cEGLConfig& x, const cEGLConfig& y ){
	return x.stencil() > y.stencil();
};

void cEGLConfigList::sort_by_stencil( const bool up_order ){
	G_ENTER();
	m_list.sort( ( up_order )? pred_stencil_up : pred_stencil_down );
	G_EXIT();
};

bool pred_samples_up( const cEGLConfig& x, const cEGLConfig& y ){
	return y.samples() > x.samples();
};

bool pred_samples_down( const cEGLConfig& x, const cEGLConfig& y ){
	return x.samples() > y.samples();
};

void cEGLConfigList::sort_by_samples( const bool up_order ){
	G_ENTER();
	m_list.sort( ( up_order )? pred_samples_up : pred_samples_down );
	G_EXIT();
};

bool pred_caveat_up( const cEGLConfig& x, const cEGLConfig& y ){
	return y.caveat() > x.caveat();
};

void cEGLConfigList::sort_by_caveat(){
	G_ENTER();
	m_list.sort( pred_caveat_up );
	G_EXIT();
};