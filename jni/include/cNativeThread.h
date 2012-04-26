#pragma once

#include <pthread.h>
#include "cNativeMutex.h"

typedef pthread_t	tid_t;

class cNativeThread {
private:
	pthread_t		m_tid;
	bool			m_running;
	bool			m_terminated;

	cCritSection	m_locker;

	static void* thread_routine( void* );
	static void on_thread_terminate( void* );

protected:	
	virtual const bool thread_entry() = 0;
	virtual void on_start();
	virtual void on_stop();

public:
	cNativeThread();
	virtual ~cNativeThread();

	void run();
	void stop();

	const bool is_created() const { return 0 != m_tid; };
	const bool is_running() const { return m_running; };
	const bool is_terminated() const { return m_terminated; };
	const bool is_self() const { return current_thread_id() == m_tid; };

	static const tid_t current_thread_id() { return (tid_t)pthread_self(); };
	static cNativeThread* current_thread();
	static void stop_all();
};
