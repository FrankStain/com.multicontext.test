#include "cNaiveThread.h"
#include "logs.h"
#include <list>
#include <algorithm>

using namespace std;

#ifdef ENABLE_THREAD_LOGS
#define LOG_PTH_I			NT_LOG_INFO
#define LOG_PTH_D			NT_LOG_DBG
#define LOG_PTH_W			NT_LOG_WARN
#define LOG_PTH_V			NT_LOG_VERB
#else
#define LOG_PTH_I(...)
#define LOG_PTH_D(...)
#define LOG_PTH_W(...)
#define LOG_PTH_V(...)
#endif

#define LOG_PTH_E			NT_LOG_ERR
#define LOG_PTH_START(...)	LOG_PTH_V( "ENTER" )
#define LOG_PTH_END(...)	LOG_PTH_V( "EXIT" )

typedef list<cNativeThread*>	thread_list_t;

namespace thread_manager {
	thread_list_t	g_threads;
};

cNativeThread::cNativeThread(): m_tid(NULL), m_running(false), m_terminated(false) {
	LOG_PTH_START();
	thread_manager::g_threads.push_back( this );
	LOG_PTH_END();
};

cNativeThread::~cNativeThread(){
	LOG_PTH_START();
	stop();
	m_locker.enter();
	thread_list_t::iterator t = find( thread_manager::g_threads.begin(), thread_manager::g_threads.end(), this );
	if( thread_manager::g_threads.end() != t ){
		LOG_PTH_I( "remove thread #0x%8X from threads list", m_tid );
		thread_manager::g_threads.erase( t );
	};
	m_locker.exit();
	LOG_PTH_END();
};

void cNativeThread::run(){
	LOG_PTH_START();
	if( !( is_running() || is_created() ) ){
		m_running		= false;
		m_terminated	= false;

		LOG_PTH_I( "make new thread" );
		pthread_create( &m_tid, 0, thread_routine, this );
	};
	LOG_PTH_END();
};

void cNativeThread::stop(){
	LOG_PTH_START();
	m_terminated = true;
	if( is_created() && !is_self() ){
		LOG_PTH_I( "waiting until thread stoped" );
		pthread_join( m_tid, 0 );
	};
	LOG_PTH_END();
};

void cNativeThread::on_start(){
	LOG_PTH_START();
	LOG_PTH_END();
};

void cNativeThread::on_stop(){
	LOG_PTH_START();
	LOG_PTH_END();
};

cNativeThread* cNativeThread::current_thread(){
	LOG_PTH_START();
	const tid_t current_tid = current_thread_id();
	cNativeThread* thread = NULL;
	for( thread_list_t::iterator t = thread_manager::g_threads.begin(); thread_manager::g_threads.end() != t; t++ ){
		if( current_tid == (*t)->m_tid ){
			thread = (*t);
			break;
		};
	};
	LOG_PTH_END();
	return thread;
};

void* cNativeThread::thread_routine( void* param ){
	LOG_PTH_START();
	cNativeThread& thread = *(cNativeThread*)param;
	thread.m_locker.enter();

	LOG_PTH_I( "thread [0x%08X; TID:0x%08X] start works", param, thread.m_tid );
	pthread_cleanup_push( &on_thread_terminate, param );

	LOG_PTH_I( "invoke on_start()" );
	thread.on_start();

	LOG_PTH_I( "enter to thread routine" );
	thread.m_running = true;
	while( !thread.m_terminated && thread.thread_entry() ){
	
	};
	LOG_PTH_I( "leave thread routine" );

	LOG_PTH_I( "thread [0x%08X; TID:0x%08X] end works", param, thread.m_tid );
	pthread_cleanup_pop( 1 );
	
	LOG_PTH_END();
	return 0;
};

void cNativeThread::on_thread_terminate( void* param ){
	LOG_PTH_START();
	cNativeThread& thread = *(cNativeThread*)param;

	LOG_PTH_I( "invoke on_stop()" );
	thread.m_running = false;
	thread.on_stop();
	thread.m_tid = NULL;
		
	LOG_PTH_END();
	thread.m_locker.exit();
};

void cNativeThread::stop_all(){
	LOG_PTH_START();
	for_each( thread_manager::g_threads.begin(), thread_manager::g_threads.end(), mem_fun( &cNativeThread::stop ) );
	LOG_PTH_END();
};
