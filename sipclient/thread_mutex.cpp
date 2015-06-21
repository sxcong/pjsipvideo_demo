#include "stdafx.h"
#include "thread_mutex.h"
#include <stddef.h>

// class Thread_Mutex
Thread_Mutex::Thread_Mutex()
{
	pthread_mutex_init(&m_thread_mutex, NULL);
}

Thread_Mutex::~Thread_Mutex()
{
	pthread_mutex_destroy(&m_thread_mutex);

}

int Thread_Mutex::acquire()
{
	int rc = pthread_mutex_lock(&m_thread_mutex);
	if (0 != rc) {
		return -1;
	}
	return 0;

}

int Thread_Mutex::release()
{
	int rc = pthread_mutex_unlock(&m_thread_mutex);
	if (0 != rc) {
		return -1;
	}
	return 0;
}


// class Thread_Mutex_Guard
Thread_Mutex_Guard::Thread_Mutex_Guard(Thread_Mutex* mutex) : m_mutex(mutex)
{
	m_mutex->acquire();
}

Thread_Mutex_Guard::~Thread_Mutex_Guard()
{
	m_mutex->release();
}