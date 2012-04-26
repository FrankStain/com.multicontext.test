#pragma once

#include <jni.h>
#include <errno.h>
#include <android/log.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/looper.h>
#include <android/configuration.h>
#include <android/native_activity.h>

#include "logs.h"
#include "cNativeMutex.h"

union cNativeApplicationState
{
	int	m_flags;
	struct {
		bool	m_running : 1;
		bool	m_paused : 1;
		bool	m_terminated : 1;
		bool	m_full_stopped : 1;
		bool	m_term_status : 1;
		bool	m_state_saved : 1;
		bool	m_need_redraw : 1;
		bool	m_soft_keypad_on : 1;

		bool	m_has_window : 1;
		bool	m_has_ogl_surface : 1;
	};

	cNativeApplicationState( int v ) : m_flags(v) {};
	cNativeApplicationState() : m_flags(0) {};
	inline operator int&() { return m_flags; };
};

enum eAppMessage
{
	AM_UNDEFINED		= 0x00,
	AM_APP_START		= 0x01,
	AM_APP_STOP,
	AM_APP_PAUSE,
	AM_APP_RESUME,
	AM_APP_DESTROY,
	AM_APP_SAVE_STATE,
	AM_CONFIG_CHANGE,
	AM_LOW_MEMORY,
	AM_WINDOW_FOCUS_CHANGE,
	AM_WINDOW_CHANGE,
	AM_WINDOW_RESIZE,
	AM_INPUT_CHANGE,
};

union sAppMsgParam
{
	int		m_value;
	void*	m_pointer;

	sAppMsgParam( int v ) : m_value(v) {};
	sAppMsgParam( void* v ) : m_pointer(v) {};
	inline operator int&() { return m_value; };
};

struct sAppMessage
{
	eAppMessage		m_command;
	sAppMsgParam	m_low;
	sAppMsgParam	m_high;

	sAppMessage() : m_command(AM_UNDEFINED), m_low(0), m_high(0) {};
	sAppMessage( eAppMessage c, sAppMsgParam l = 0, sAppMsgParam h = 0 ) : m_command(c), m_low(l), m_high(h) {};
};

static const size_t APP_MESSAGE_SIZE = sizeof( sAppMessage );

class cCommandProcessor;
class cInputProcessor;

class cNativeApplication
{
private:	
	ANativeActivity*	m_activity;
	AConfiguration*		m_config;
	ALooper*			m_looper;
	AInputQueue*		m_input_driver;
	ANativeWindow*		m_window;
	void*				m_saved_state;
	size_t				m_state_size;
	cCommandProcessor*	m_cmd_processor;
	cInputProcessor*	m_input_processor;

	int					m_pipe_read;
	int					m_pipe_write;
	int					m_actual_state;

	cNativeApplicationState	m_state;
		
	pthread_t			m_thread;
	cMonitor			m_monitor;
												 
protected:
	cNativeApplication();
	virtual ~cNativeApplication();

	static void* app_thread_entry( void* );
	
	static void on_app_start( ANativeActivity* );
	static void on_app_stop( ANativeActivity* );
	static void on_app_pause( ANativeActivity* );
	static void on_app_resume( ANativeActivity* );
	static void on_app_destroy( ANativeActivity* );
	static void* on_save_app_state( ANativeActivity*, size_t* );
	static void on_change_config( ANativeActivity* );
	static void on_low_memory( ANativeActivity* );
	static void on_change_window_focus( ANativeActivity*, int );
	static void on_create_native_window( ANativeActivity*, ANativeWindow* );
	static void on_destroy_native_window( ANativeActivity*, ANativeWindow* );
	static void on_resize_native_window( ANativeActivity*, ANativeWindow* );
	static void on_create_input_queue( ANativeActivity*, AInputQueue* );
	static void on_destroy_input_queue( ANativeActivity*, AInputQueue* );

	bool process_commands();
	bool preprocess_command( sAppMessage& );
	bool postprocess_command( sAppMessage& );
	bool process_input();
	void set_app_state( const eAppMessage );

	void release();
	
public:
	static cNativeApplication* instance();
	static const bool process_messages( int = 0 );

	const cNativeApplicationState& state() { return m_state; };
	
	const bool construct( ANativeActivity*, void*, size_t );
	const bool post_message( const sAppMessage& );
	
	void set_command_processor( cCommandProcessor* cp ) { m_cmd_processor = cp; };
	void set_input_processor( cInputProcessor* ip ) { m_input_processor = ip; };
};

class cCommandProcessor
{
public:
	virtual ~cCommandProcessor() {};

	virtual const bool process_command( const sAppMessage&, cNativeApplication& ) = 0;
};

class cInputProcessor
{
public:
	virtual ~cInputProcessor() {};

	virtual const bool process( const AInputEvent&, cNativeApplication& ) = 0;
};

extern void android_main( cNativeApplication& );