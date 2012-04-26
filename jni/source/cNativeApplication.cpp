#include "cNativeApplication.h"
#include <unistd.h>
#include <sys/resource.h>
#include "cGraphics.h"

#ifndef PTR_DELETE
#define PTR_DELETE(P) { delete P; P = NULL; }
#endif

#ifdef ENABLE_ACTIVITY_LOGS
#define LOG_ACT_I			NT_LOG_INFO
#define LOG_ACT_D			NT_LOG_DBG
#define LOG_ACT_W			NT_LOG_WARN
#define LOG_ACT_V			NT_LOG_VERB
#else
#define LOG_ACT_I(...)
#define LOG_ACT_D(...)
#define LOG_ACT_W(...)
#define LOG_ACT_V(...)
#endif

#define LOG_ACT_E			NT_LOG_ERR
#define LOG_ACT_START(...)	LOG_ACT_V( "ENTER" )
#define LOG_ACT_END(...)	LOG_ACT_V( "EXIT" )

#ifndef PTR_DELETE
#define PTR_DELETE(P) { delete P; P = NULL; }
#endif

enum eMessagePipeIdent
{
	piCommand	= 1,
	piInput		= 2,
	piUser		= 1024,
};

void ANativeActivity_onCreate( ANativeActivity* activity, void* savedState, size_t savedStateSize )
{
	LOG_ACT_START();
	cNativeApplication::instance()->construct( activity, savedState, savedStateSize );
	LOG_ACT_END();
};

namespace {
	cNativeApplication* app_instance = NULL;
};

cNativeApplication::cNativeApplication():
	m_activity(NULL), m_config(NULL), m_looper(NULL), m_input_driver(NULL), m_window(NULL), m_saved_state(NULL), 
	m_state_size(0), m_cmd_processor(NULL), m_input_processor(NULL), m_pipe_read(0), m_pipe_write(0), 
	m_actual_state(0), m_state(0)
{
	LOG_ACT_START();
	app_instance = this;
	LOG_ACT_END();
};

cNativeApplication::~cNativeApplication()
{
	LOG_ACT_START();
	m_cmd_processor		= NULL;
	m_input_processor	= NULL;
	//PTR_DELETE( m_saved_state );
	app_instance = NULL;
	LOG_ACT_END();
};

cNativeApplication* cNativeApplication::instance()
{
	return ( app_instance )? app_instance : new cNativeApplication();
};

const bool cNativeApplication::construct( ANativeActivity* activity, void* state, size_t state_size )
{
	LOG_ACT_START();
	
	m_state			= 0;
	m_actual_state	= 0;
	m_activity		= activity;
	if( state )
	{
		m_state_size	= state_size;
		LOG_ACT_I( "%d bytes allocated for saved state", m_state_size );
		m_saved_state	= new char[ state_size ];
		memcpy( m_saved_state, state, m_state_size );
	};
		
	LOG_ACT_I( "connect structures" );
	m_activity->instance				= this;
	ANativeActivityCallbacks &cb	= *m_activity->callbacks;
	
	LOG_ACT_I( "fill callbacks" );
	cb.onStart					= on_app_start;
	cb.onStop					= on_app_stop;
	cb.onPause					= on_app_pause;
	cb.onResume					= on_app_resume;
	cb.onDestroy				= on_app_destroy;
	cb.onSaveInstanceState		= on_save_app_state;
	cb.onConfigurationChanged	= on_change_config;
	cb.onLowMemory				= on_low_memory;
	cb.onWindowFocusChanged		= on_change_window_focus;
	cb.onNativeWindowCreated	= on_create_native_window;
	cb.onNativeWindowDestroyed	= on_destroy_native_window;
	cb.onNativeWindowResized	= on_resize_native_window;
	cb.onInputQueueCreated		= on_create_input_queue;
	cb.onInputQueueDestroyed	= on_destroy_input_queue;

	LOG_ACT_I( "create message pipe" );
	int fd_pipe[2] = {0};
	if( pipe( fd_pipe ) )
	{
		LOG_ACT_E( "Can not allocate pipe resources." );
		return false;
	};

	m_pipe_read		= fd_pipe[0];
	m_pipe_write	= fd_pipe[1];

	LOG_ACT_I( "create main thread" );
	pthread_attr_t	attr;
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	pthread_create( &m_thread, &attr, app_thread_entry, m_activity );

	LOG_ACT_I( "waiting until main thread starts" );
	m_monitor.enter();
	while( !m_state.m_running )
	{
		m_monitor.wait();
	};
	m_monitor.exit();

	LOG_ACT_END();
	return true;
};

void cNativeApplication::release()
{
	LOG_ACT_START();
	ANativeActivityCallbacks &cb	= *m_activity->callbacks;
	memset( &cb, 0, sizeof( ANativeActivityCallbacks ) );

	close( m_pipe_write );
	close( m_pipe_read );
		
	m_activity->instance		= NULL;
	m_state					= 0;
	m_actual_state			= 0;
	LOG_ACT_END();
};

void* cNativeApplication::app_thread_entry( void* user_data )
{
	LOG_ACT_START();
	ANativeActivity& activity = *(ANativeActivity*)user_data;
	cNativeApplication& app = *(cNativeApplication*)activity.instance;
	
	LOG_ACT_I( "get the application config" );
	app.m_config = AConfiguration_new();
	AConfiguration_fromAssetManager( app.m_config, activity.assetManager );

	LOG_ACT_I( "prepare message looper" );
	app.m_looper = ALooper_prepare( ALOOPER_PREPARE_ALLOW_NON_CALLBACKS );
	ALooper_addFd( app.m_looper, app.m_pipe_read, piCommand, ALOOPER_EVENT_INPUT, NULL, &app );
	
	LOG_ACT_I( "enable 'running' state" );
	app.m_monitor.enter();
	app.m_state.m_running = true;
	app.m_state.m_full_stopped = false;
	app.m_monitor.notify();
	app.m_monitor.exit();
	
	LOG_ACT_I( "init graphics (inst : 0x%08X)", cGraphics::instance() );
	cGraphics::instance()->init();
	LOG_ACT_I( "invoke 'android_main' routine" );
	android_main( app );
	LOG_ACT_I( "'android_main' routine finished" );
	cGraphics::instance()->final();
		
	LOG_ACT_I( "disable 'running' state" );
	app.m_state.m_running = false;
		
	if( app.m_input_driver )
	{
		LOG_ACT_I( "detaching input driver" );
		AInputQueue_detachLooper( app.m_input_driver );
		app.m_input_driver = NULL;
	};
	
	LOG_ACT_I( "destroy the application config" );
	AConfiguration_delete( app.m_config );
	app.m_config = NULL;

	LOG_ACT_I( "set the 'terminated' state" );
	app.m_monitor.enter();
	app.m_state.m_full_stopped = true;
	app.m_monitor.notify_all();
	app.m_monitor.exit();
	
	LOG_ACT_END();
	return NULL;
};

const bool cNativeApplication::process_messages( int msec_timeout )
{
	int ident = -1;
	int event_count = 0;
	cNativeApplication* app = NULL;

	do
	{
		ident = ALooper_pollAll( msec_timeout, NULL, &event_count, (void**)&app );
		
		if( app && ( 0 <= ident ) )
		{
			LOG_ACT_START();
			LOG_ACT_I( "looper ident == %d", ident );
			switch( ident )
			{
				case piCommand:
					app->process_commands();
				break;
				case piInput:
					app->process_input();
				break;
				default:
					
				break;
			};

			LOG_ACT_END();
			return true;
		};

	} while( 0 <= ident );

	return false;
};

bool cNativeApplication::process_commands()
{
	LOG_ACT_START();
	sAppMessage msg;

	m_state.m_state_saved = false;
	if( APP_MESSAGE_SIZE == read( m_pipe_read, &msg, APP_MESSAGE_SIZE ) ){
		preprocess_command( msg );
		if( m_cmd_processor )
		{
			LOG_ACT_I( "invoke command processor" );
			m_cmd_processor->process_command( msg, *this );
		}; 
		postprocess_command( msg );
		LOG_ACT_END();
		return true;
	};

	LOG_ACT_W( "EXIT" );
	return false;
};

bool cNativeApplication::preprocess_command( sAppMessage& msg )
{
	LOG_ACT_START();
	switch( msg.m_command )
	{
		case AM_INPUT_CHANGE:
			LOG_ACT_D( "[AM_INPUT_CHANGE] message processiing started" );
			m_monitor.enter();

			if( m_input_driver )
			{
				LOG_ACT_I( "Flush old input driver" );
				AInputQueue_detachLooper( m_input_driver );
			};

			m_input_driver = (AInputQueue*)msg.m_low.m_pointer;

			if( m_input_driver )
			{
				LOG_ACT_I( "set new input driver" );
				AInputQueue_attachLooper( m_input_driver, m_looper, piInput, NULL, this );
			};

			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_INPUT_CHANGE] message processiing finished" );
		break;
		case AM_WINDOW_CHANGE:
			LOG_ACT_D( "[AM_WINDOW_CHANGE] message processiing started" );
			LOG_ACT_I( "m_window 0x%08X; msg.m_low 0x%08X", m_window, msg.m_low.m_value );
			if( !m_window && msg.m_low.m_pointer )
			{
				cCSLock lock( m_monitor );

				LOG_ACT_I( "Set new native window" );
				m_window = (ANativeWindow*)msg.m_low.m_pointer;
				m_state.m_has_window = false;
				m_state.m_has_ogl_surface = cGraphics::instance()->set_window( m_window );
				m_monitor.notify();
			};
			LOG_ACT_D( "[AM_WINDOW_CHANGE] message processiing finished" );
		break;
		case AM_WINDOW_RESIZE:
			LOG_ACT_D( "[AM_WINDOW_RESIZE] message processiing started" );
			LOG_ACT_I( "new window dimensions : %dx%d", msg.m_low.m_value, msg.m_high.m_value );
			LOG_ACT_D( "[AM_WINDOW_RESIZE] message processiing finished" );
		break;
		case AM_APP_START:
			LOG_ACT_D( "[AM_APP_START] message processiing started" );
			m_monitor.enter();
			m_state.m_running	= true;
			m_actual_state		= msg.m_command;
			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_APP_START] message processiing finished" );
		break;
		case AM_APP_STOP:
			LOG_ACT_D( "[AM_APP_STOP] message processiing started" );
			m_monitor.enter();
			m_state.m_running	= false;
			m_actual_state		= msg.m_command;
			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_APP_STOP] message processiing finished" );
		break;
		case AM_APP_PAUSE:
			LOG_ACT_D( "[AM_APP_PAUSE] message processiing started" );
			m_monitor.enter();
			m_state.m_paused	= true;
			m_actual_state		= msg.m_command;
			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_APP_PAUSE] message processiing finished" );
		break;
		case AM_APP_RESUME:
			LOG_ACT_D( "[AM_APP_RESUME] message processiing started" );
			m_monitor.enter();
			m_state.m_paused	= false;
			m_actual_state		= msg.m_command;
			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_APP_RESUME] message processiing finished" );
		break;
		case AM_APP_DESTROY:
			LOG_ACT_D( "[AM_APP_DESTROY] message processiing started" );
			m_state.m_term_status = true;
			LOG_ACT_D( "[AM_APP_DESTROY] message processiing finished" );
		break;
		case AM_CONFIG_CHANGE:
			LOG_ACT_D( "[AM_CONFIG_CHANGE] message processiing started" );
			AConfiguration_fromAssetManager( m_config, m_activity->assetManager );
			LOG_ACT_D( "[AM_CONFIG_CHANGE] message processiing finished" );
		break;
	};

	LOG_ACT_END();
	return true;
};

bool cNativeApplication::postprocess_command( sAppMessage& msg )
{
	LOG_ACT_START();
	switch( msg.m_command )
	{
		case AM_WINDOW_CHANGE:
			LOG_ACT_D( "[AM_WINDOW_CHANGE] message processiing started" );
			if( m_window && m_state.m_has_window )
			{
				cCSLock lock( m_monitor );

				LOG_ACT_I( "Change native window" );
				m_window = (ANativeWindow*)msg.m_low.m_pointer;
				
				if( m_state.m_has_ogl_surface ){
					cGraphics::instance()->unset_window();
					m_state.m_has_ogl_surface = false;
				};
				
				if( m_window ){
					m_state.m_has_ogl_surface = cGraphics::instance()->set_window( m_window );
				};

				m_monitor.notify();
			}else{
				m_state.m_has_window = NULL != m_window;
			};
			LOG_ACT_D( "[AM_WINDOW_CHANGE] message processiing started" ); 
		break;
		case AM_APP_SAVE_STATE:
			LOG_ACT_D( "[AM_APP_SAVE_STATE] message processiing started" );
			m_monitor.enter();
			m_state.m_state_saved = true;
			m_monitor.notify();
			m_monitor.exit();
			LOG_ACT_D( "[AM_APP_SAVE_STATE] message processiing started" );	
		break;
		case AM_APP_RESUME:
			LOG_ACT_D( "[AM_APP_RESUME] message processiing started" );
			m_state.m_state_saved = false;
			LOG_ACT_D( "[AM_APP_RESUME] message processiing started" );	 
		break;
		case AM_APP_DESTROY:
			LOG_ACT_D( "[AM_APP_DESTROY] message processiing started" );
			m_state.m_terminated = true;
			m_state.m_term_status = false;
			LOG_ACT_D( "[AM_APP_DESTROY] message processiing finished" );
		break;
	};
	LOG_ACT_END();
	return true;
};

bool cNativeApplication::process_input()
{
	LOG_ACT_START();
	AInputEvent* ev = NULL;
	if( 0 <= AInputQueue_getEvent( m_input_driver, &ev ) )
	{
		int ev_type = AInputEvent_getType( ev );
		
		if( AInputQueue_preDispatchEvent( m_input_driver, ev ) )
		{
			LOG_ACT_I( "input event type : 0x%08X", ev_type );
			LOG_ACT_W( "EXIT" );
			return false;
		};

		switch( ev_type )
		{
			case AINPUT_EVENT_TYPE_KEY:{
				LOG_ACT_D( "[AINPUT_EVENT_TYPE_KEY] event start" );

				int kf = AKeyEvent_getFlags( ev );
				int kc = AKeyEvent_getKeyCode( ev );
				int sc = AKeyEvent_getScanCode( ev );

				LOG_ACT_I( "flags : 0x%08X", kf );
				LOG_ACT_I( "key code : %c | %C | 0x%02X | 0x%04X", kc, kc, kc, kc );
				LOG_ACT_I( "scan code : %c | %C | 0x%02X | 0x%04X", sc, sc, sc, sc );

				switch( AKeyEvent_getAction( ev ) )
				{
					case AKEY_EVENT_ACTION_DOWN:
						LOG_ACT_D( "[AKEY_EVENT_ACTION_DOWN] action start" );

						LOG_ACT_D( "[AKEY_EVENT_ACTION_DOWN] action finish" );
					break;
					case AKEY_EVENT_ACTION_UP:
						LOG_ACT_D( "[AKEY_EVENT_ACTION_DOWN] action start" );

						LOG_ACT_D( "[AKEY_EVENT_ACTION_DOWN] action finish" );
					break;
					default:
						LOG_ACT_W( "[0x%08X] action catch", AKeyEvent_getAction( ev ) );
					break;
				};
				LOG_ACT_D( "[AINPUT_EVENT_TYPE_KEY] event finish" );
			}break;
			case AINPUT_EVENT_TYPE_MOTION:
				LOG_ACT_D( "[AINPUT_EVENT_TYPE_MOTION] event stert" );

				LOG_ACT_D( "[AINPUT_EVENT_TYPE_MOTION] event finish" );
			break;
		};

		if( m_input_processor )
		{			
			LOG_ACT_I( "invoke input processor" );
			m_input_processor->process( *ev, *this );
		};

		LOG_ACT_I( "finish event processing" );
		AInputQueue_finishEvent( m_input_driver, ev, 0 );
		
		LOG_ACT_I( "EXIT" );
		return true;
	};

	LOG_ACT_W( "EXIT" );
	return false;
};

const bool cNativeApplication::post_message( const sAppMessage& msg )
{
	return APP_MESSAGE_SIZE == write( m_pipe_write, &msg, APP_MESSAGE_SIZE );
};

void cNativeApplication::set_app_state( const eAppMessage state )
{
	LOG_ACT_START();
	cCSLock lock( m_monitor );
	post_message( sAppMessage( state ) );
	while( state != m_actual_state ){
		m_monitor.wait();
	};
	LOG_ACT_END();
};

void cNativeApplication::on_app_start( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.set_app_state( AM_APP_START );
	LOG_ACT_END();
};

void cNativeApplication::on_app_stop( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.set_app_state( AM_APP_STOP );
	LOG_ACT_END();
};

void cNativeApplication::on_app_pause( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.set_app_state( AM_APP_PAUSE );
	LOG_ACT_END();
};

void cNativeApplication::on_app_resume( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.set_app_state( AM_APP_RESUME );
	LOG_ACT_END();
};

void cNativeApplication::on_app_destroy( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.m_monitor.enter();
	app.post_message( sAppMessage( AM_APP_DESTROY ) );
	while( !app.m_state.m_full_stopped )
	{
		app.m_monitor.wait();
	};
	app.m_monitor.exit();

	app.release();
	delete app_instance;
	ANativeActivity_finish( activity );

	LOG_ACT_END();
};

void* cNativeApplication::on_save_app_state( ANativeActivity* activity, size_t* state_size )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	cCSLock lock( app.m_monitor );
	size_t& size = *state_size;
	void* app_state = NULL;
	size = 0;

	LOG_ACT_I( "send notification to thread" );
	app.post_message( sAppMessage( AM_APP_SAVE_STATE, state_size ) );
	while( !app.m_state.m_state_saved )
	{
		app.m_monitor.wait();
	};

	if( app.m_saved_state )
	{
	
	};

	LOG_ACT_END();
	return app_state;
};

void cNativeApplication::on_change_config( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.post_message( sAppMessage( AM_CONFIG_CHANGE ) );
	LOG_ACT_END();
};

void cNativeApplication::on_low_memory( ANativeActivity* activity )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.post_message( sAppMessage( AM_LOW_MEMORY ) );
	LOG_ACT_END();
};

void cNativeApplication::on_change_window_focus( ANativeActivity* activity, int focus )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	app.post_message( sAppMessage( AM_WINDOW_FOCUS_CHANGE, focus ) );
	LOG_ACT_END();
};

void cNativeApplication::on_create_native_window( ANativeActivity* activity, ANativeWindow* window )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	cCSLock lock( app.m_monitor );

	app.post_message( sAppMessage( AM_WINDOW_CHANGE, window ) );
	while( window != app.m_window )
	{
		app.m_monitor.wait();
	};

	LOG_ACT_END();
};

void cNativeApplication::on_destroy_native_window( ANativeActivity* activity, ANativeWindow* window )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	cCSLock lock( app.m_monitor );

	app.post_message( sAppMessage( AM_WINDOW_CHANGE, NULL ) );
	while( app.m_window )
	{
		app.m_monitor.wait();
	};

	LOG_ACT_END();
};

void cNativeApplication::on_resize_native_window( ANativeActivity* activity, ANativeWindow* window )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	int w = ANativeWindow_getWidth( window );
	int h = ANativeWindow_getHeight( window );

	LOG_ACT_I( "window size : %dx%d", w, h );
	app.post_message( sAppMessage( AM_WINDOW_RESIZE, w, h ) );

	LOG_ACT_END();
};

void cNativeApplication::on_create_input_queue( ANativeActivity* activity, AInputQueue* queue )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	cCSLock lock( app.m_monitor );

	app.post_message( sAppMessage( AM_INPUT_CHANGE, queue ) );
	while( queue != app.m_input_driver )
	{
		app.m_monitor.wait();
	};

	LOG_ACT_END();
};

void cNativeApplication::on_destroy_input_queue( ANativeActivity* activity, AInputQueue* queue )
{
	LOG_ACT_START();
	cNativeApplication& app = *(cNativeApplication*)activity->instance;
	cCSLock lock( app.m_monitor );

	app.post_message( sAppMessage( AM_INPUT_CHANGE, NULL ) );
	while( app.m_input_driver )
	{
		app.m_monitor.wait();
	};

	LOG_ACT_END();
};
