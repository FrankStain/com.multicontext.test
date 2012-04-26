#pragma once

#include <pthread.h>
#include <sched.h>

#include "logs.h"

class cNativeMutex {
protected:
	pthread_mutex_t	m_mutex;

public:
	cNativeMutex() {};
	virtual ~cNativeMutex() {};

	virtual void enter() { pthread_mutex_lock( &m_mutex ); };
	virtual void exit() { pthread_mutex_unlock( &m_mutex ); };
};

class cNativeCS : public cNativeMutex {
protected:
	pthread_mutexattr_t	m_attr;

public:
	cNativeCS(){
		pthread_mutexattr_init( &m_attr );
		pthread_mutexattr_settype( &m_attr, PTHREAD_MUTEX_RECURSIVE );
		pthread_mutex_init( &m_mutex, &m_attr );
	};

	virtual ~cNativeCS(){
		pthread_mutex_destroy( &m_mutex );
		pthread_mutexattr_destroy( &m_attr );
	};
};

class cNativeMonitor : public cNativeMutex {
protected:
	pthread_cond_t	m_cond;

public:
	cNativeMonitor(){
		pthread_mutex_init( &m_mutex, NULL );
		pthread_cond_init( &m_cond, NULL );
	};

	virtual ~cNativeMonitor(){
		pthread_cond_destroy( &m_cond );
		pthread_mutex_destroy( &m_mutex );
	};

	void wait() { pthread_cond_wait( &m_cond, &m_mutex ); };
	void notify() { pthread_cond_signal( &m_cond ); };
	void notify_all() { pthread_cond_broadcast( &m_cond ); };
};

class cNativeMutexLock
{
private:
	cNativeMutex*	m_cs;

public:
	cNativeMutexLock( cNativeMutex& cs ): m_cs(&cs) { if( m_cs ){ m_cs->enter(); }; };
	virtual ~cNativeMutexLock() { if( m_cs ){ m_cs->exit(); }; };

};

typedef cNativeCS			cCritSection;
typedef cNativeMonitor		cMonitor;
typedef cNativeMutexLock	cCSLock;