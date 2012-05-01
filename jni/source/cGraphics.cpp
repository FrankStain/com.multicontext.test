#include "cGraphics.h"
#include "cConfigurator.h"

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

cGraphics* cGraphics::m_instance = NULL;

bool g_extensions_checked	= false;
bool g_ext_mapbuffer		= false;
bool g_ext_bufferobject		= true;

typedef void* (*MapBufferOES_t) ( GLenum target, GLenum access );
typedef GLboolean (*UnmapBufferOES_t) ( GLenum target );

void* glMapBufferOES( GLenum target, GLenum access )
{
	static MapBufferOES_t proc = 0;
	static bool can_be_loaded = true;

	if( !proc && can_be_loaded )
	{
		proc = (MapBufferOES_t)eglGetProcAddress( "glMapBufferOES" );

		if( !proc )
		{
			can_be_loaded = false;
		};
	};

	return ( proc )? proc( target, access ) : 0;
};

GLboolean glUnmapBufferOES( GLenum target )
{
	static UnmapBufferOES_t proc = 0;
	static bool can_be_loaded = true;

	if( !proc && can_be_loaded )
	{
		proc = (UnmapBufferOES_t)eglGetProcAddress( "glUnmapBufferOES" );

		if( !proc )
		{
			can_be_loaded = false;
		};
	};

	return ( proc )? proc( target ) : false;
};

cGraphics::cGraphics() : m_host_thread(0), m_lock_thread(0), m_display(EGL_NO_DISPLAY), m_render(), m_storage() {
	G_ENTER();
	m_instance = this;
	G_EXIT();
};

cGraphics::~cGraphics(){
	G_ENTER();
	final();
	m_instance = NULL;
	G_EXIT();
};

const bool cGraphics::init(){
	G_ENTER();
	if( EGL_NO_DISPLAY != m_display ){
		G_FAIL();
		return false;
	};

	G_LOG_I( "get current display" );
	m_display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	eglBindAPI( EGL_OPENGL_ES_API );

	if( EGL_NO_DISPLAY == m_display ){
		G_LOG_E( "eglGetDisplay() failed [code : 0x%08X]", eglGetError() );
		return false;
	};

	G_LOG_I( "try to init egl" );
	memset( m_version, 0, sizeof( m_version ) );
	if( !eglInitialize( m_display, &(m_version[1]), &(m_version[0]) ) ){
		G_LOG_E( "eglInitialize() failed [code : 0x%08X]", eglGetError() );
		return false;
	};

	m_host_thread = pthread_self();

	G_LOG_I( "--- EGL environment data ---" );
	G_LOG_I( "EGL_VENDOR     : '%s'", eglQueryString( m_display, EGL_VENDOR ) );
	G_LOG_I( "EGL_VERSION    : '%s'", eglQueryString( m_display, EGL_VERSION ) );
	G_LOG_I( "EGL_EXTENSIONS : '%s'", eglQueryString( m_display, EGL_EXTENSIONS ) );
	G_LOG_I( "--- -------------------- ---" );

	G_LOG_I( "get EGL configs" );
	cEGLConfigList configs( &m_display );
	//configs.print();

	configs.filter_in_native_visual_id( WINDOW_FORMAT_RGBA_8888 );	//configs.print();
	configs.filter_in_gl_version( EGL_OPENGL_ES_BIT );				//configs.print();
	configs.filter_in_surface_type( EGL_WINDOW_BIT );				//configs.print();
	configs.filter_in_buffer_size( 32 );							//configs.print();
	configs.filter_in_rgba( 8, 8, 8, 8 );							//configs.print();

	configs.sort_by_stencil();										//configs.print();
	configs.sort_by_depth();										//configs.print();
	configs.sort_by_caveat();										//configs.print();
		
	G_LOG_I( "selected rendering config : " );
	configs[0].print();
	m_render.m_config = *configs[0].config();

#ifdef USE_MULTICONTEXT
	configs.filter_flush();
	configs.filter_in_native_visual_id( WINDOW_FORMAT_RGBX_8888 );	//configs.print();
	configs.filter_in_gl_version( EGL_OPENGL_ES_BIT );				//configs.print();
	configs.filter_in_surface_type( EGL_PBUFFER_BIT );				//configs.print();
	configs.filter_in_buffer_size( 32 );							//configs.print();
	configs.filter_in_rgb( 8, 8, 8 );								//configs.print();

	configs.sort_by_stencil( true );								//configs.print();
	configs.sort_by_depth( true );									//configs.print();
	configs.sort_by_caveat();										//configs.print();

	G_LOG_I( "selected storage config : " );
	configs[0].print();
	m_storage.m_config = *configs[0].config();
#endif

	EGLint context_attr[] = {
		EGL_CONTEXT_CLIENT_VERSION,		1,
		EGL_NONE
	};

	G_LOG_I( "try to create rendering context" );
	m_render.m_context = eglCreateContext( m_display, m_render.m_config, EGL_NO_CONTEXT, context_attr );
	if( EGL_NO_CONTEXT == m_render.m_context ){
		G_LOG_E( "eglCreateContext() failed with code : 0x%08X", eglGetError() );
		return false;
	};

#ifdef USE_MULTICONTEXT
	G_LOG_I( "try to create storage context" );
	m_storage.m_context = eglCreateContext( m_display, m_storage.m_config, m_render.m_context, context_attr );
	if( EGL_NO_CONTEXT == m_render.m_context ){
		G_LOG_E( "eglCreateContext() failed with code : 0x%08X", eglGetError() );
		return false;
	};

	EGLint pbuffer_config[] = {
		EGL_WIDTH,	8,
		EGL_HEIGHT,	8,
		EGL_NONE,	0
	};

	G_LOG_I( "try to create storage pbuffer surface" );
	m_storage.m_surface = eglCreatePbufferSurface( m_display, m_storage.m_config, pbuffer_config );
	if( EGL_NO_SURFACE == m_storage.m_surface ){
		G_LOG_E( "eglCreatePbufferSurface() failed with code 0x%08X", eglGetError() );
		return false;
	};
#endif

	G_EXIT();
	return true;
};

void cGraphics::lock(){
#ifdef USE_MULTICONTEXT
	const pthread_t cur_tid = pthread_self();
	if( cur_tid != m_instance->m_host_thread ){
		m_instance->m_context_lock.enter();
		m_instance->m_host_thread = cur_tid;
		eglMakeCurrent( m_instance->m_display, m_instance->m_storage.m_surface, m_instance->m_storage.m_surface, m_instance->m_storage.m_context );
		m_instance->m_storage.m_is_current = true;
	};
#endif
};

void cGraphics::unlock(){
#ifdef USE_MULTICONTEXT
	const pthread_t cur_tid = pthread_self();
	if( ( cur_tid != m_instance->m_host_thread ) && ( cur_tid == m_instance->m_lock_thread ) ){
		m_instance->m_lock_thread = 0;
		eglMakeCurrent( m_instance->m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		m_instance->m_storage.m_is_current = false;
		m_instance->m_context_lock.exit();
	};
#endif
};

void cGraphics::final(){
	G_ENTER();

	if( EGL_NO_SURFACE != m_storage.m_surface ){
		G_LOG_I( "free storage surface" );
		eglDestroySurface( m_display, m_storage.m_surface );
		m_storage.m_surface = EGL_NO_SURFACE;
	};

	if( EGL_NO_CONTEXT != m_storage.m_context ){
		G_LOG_I( "free storage context" );
		eglDestroyContext( m_display, m_storage.m_context );
		m_storage.m_context = EGL_NO_CONTEXT;
		m_storage.m_is_setup = false;
	};

	if( ( EGL_NO_SURFACE != m_render.m_surface ) && ( EGL_NO_CONTEXT != m_render.m_context ) ){
		G_LOG_I( "unset rendering context" );
		eglMakeCurrent( m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		m_render.m_is_current = false;
	};
	
	if( EGL_NO_SURFACE != m_render.m_surface ){
		G_LOG_I( "free rendering surface" );
		eglDestroySurface( m_display, m_render.m_surface );
		m_render.m_surface = EGL_NO_SURFACE;
	};

	if( EGL_NO_CONTEXT != m_render.m_context ){
		G_LOG_I( "free rendering context" );
		eglDestroyContext( m_display, m_render.m_context );
		m_render.m_context = EGL_NO_CONTEXT;
		m_render.m_is_setup = false;
	};

	m_render.m_config	= NULL;
	m_storage.m_config	= NULL;

	if( EGL_NO_DISPLAY != m_display ){
		G_LOG_I( "close display connection" );
		eglTerminate( m_display );
		m_display = EGL_NO_DISPLAY;
	};
	
	G_EXIT();
};

const bool cGraphics::set_window( ANativeWindow* window ){
	G_ENTER();

	ANativeWindow_setBuffersGeometry( window, 0, 0, WINDOW_FORMAT_RGBA_8888 );

	G_LOG_I( "try create native window surface" );
	m_render.m_surface = eglCreateWindowSurface( m_display, m_render.m_config, window, NULL );
	if( EGL_NO_SURFACE == m_render.m_surface ){
		G_LOG_E( "eglCreateWindowSurface() failed with code 0x%08X", eglGetError() );
		return false;
	}else if( EGL_NO_CONTEXT != m_render.m_context ){
		G_LOG_I( "set rendering context" );
		m_render.m_is_current = false;
		if( !eglMakeCurrent( m_display, m_render.m_surface, m_render.m_surface, m_render.m_context ) ){
			G_LOG_E( "eglMakeCurrent() failed with code 0x%08X", eglGetError() );
		}else{
			m_render.m_is_current = true;
			setup_context();
			m_render.m_is_setup = true;

			G_LOG_I( "--- OpenGL general data  ---" );
			G_LOG_I( "GL_VENDOR     : '%s'", glGetString( GL_VENDOR ) );
			G_LOG_I( "GL_RENDERER   : '%s'", glGetString( GL_RENDERER ) );
			G_LOG_I( "GL_VERSION    : '%s'", glGetString( GL_VERSION ) );
			G_LOG_I( "GL_EXTENSIONS : '%s'", glGetString( GL_EXTENSIONS ) );
			G_LOG_I( "--- -------------------- ---" );

			if( !g_extensions_checked ){
				const char* ext		= (const char*)glGetString( GL_EXTENSIONS );
				g_ext_mapbuffer		= 0 < strstr( (ext)? ext : "", "GL_OES_mapbuffer" );
				g_extensions_checked = true;
			};
		};
	};
	
	G_EXIT();
	return true;
};

const bool cGraphics::unset_window(){
	G_ENTER();	
	if( ( EGL_NO_SURFACE != m_render.m_surface ) && ( EGL_NO_CONTEXT != m_render.m_context ) ){
		G_LOG_I( "unset rendering context" );
		eglMakeCurrent( m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		m_render.m_is_current = false;
	};
	
	if( EGL_NO_SURFACE != m_render.m_surface ){
		G_LOG_I( "free rendering surface" );
		eglDestroySurface( m_display, m_render.m_surface );
		m_render.m_surface = EGL_NO_SURFACE;
	};
	G_EXIT();
	return true;
};

void cGraphics::setup_context(){
	G_ENTER();

	glDisable( GL_LIGHTING );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDepthMask( GL_TRUE );
	glEnable( GL_DEPTH_TEST );
	glShadeModel( GL_SMOOTH );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
	glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE );
	glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR );

	glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
	glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE );
	glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR );

	int32_t max_tex_units = 0;
	glGetIntegerv( GL_MAX_TEXTURE_UNITS, (GLint*)&max_tex_units );
	for( int32_t ti = 1; max_tex_units > ti; ti++ ){
		glActiveTexture( GL_TEXTURE0 + ti );
		glEnable( GL_TEXTURE_2D );

		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR );

		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR );

	};

	glActiveTexture( GL_TEXTURE0 );

	glDepthFunc( GL_LEQUAL );

	//glEnable( GL_CULL_FACE );
	//glCullFace( GL_BACK );
	//glFrontFace( GL_CW );

	glEnable( GL_COLOR_MATERIAL );
	glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR );

	int32_t viewport[4] = { 0 };
	
	glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
	G_LOG_I( "current viewport is [ %d, %d, %d, %d  ]", viewport[0], viewport[1], viewport[2], viewport[3] );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrthof( 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f );

	G_EXIT();
};

void cGraphics::swap(){
	switch( eglSwapBuffers( m_display, m_render.m_surface ) ){
		case EGL_TRUE:

		break;
		case EGL_BAD_SURFACE:
			NT_LOG_ERR( "eglSwapBuffers() returns EGL_BAD_SURFACE." );
		break;
		case EGL_BAD_DISPLAY:
			NT_LOG_ERR( "eglSwapBuffers() returns EGL_BAD_DISPLAY." );
		break;
		case EGL_NOT_INITIALIZED:
			NT_LOG_ERR( "eglSwapBuffers() returns EGL_NOT_INITIALIZED." );
		break;
	};
};
