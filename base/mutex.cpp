#include "mutex.h"

Mutex::Mutex() : count_(0)
{
#ifdef WIN32
	InitializeCriticalSection(&section_);
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int  rc = pthread_mutex_init(&section_, &attr);
    assert( rc == 0 );
#endif
}

Mutex::~Mutex ()
{
#ifdef WIN32
	DeleteCriticalSection(&section_);
#else
    int  rc = pthread_mutex_destroy(&section_);
    assert( rc != EBUSY );  // currently locked 
    assert( rc == 0 );
#endif
}

void Mutex::lock()
{
#ifdef WIN32
	EnterCriticalSection(&section_);
#else
    int  rc = pthread_mutex_lock(&section_);
//    assert( rc != EINVAL );
//    assert( rc != EDEADLK );
    assert( rc == 0 );
#endif
	count_++;
}

void Mutex::unlock()
{
	if (count_ <= 0)
	{
		printf("***** unlock %d.", count_);
		return;
	}
	count_--;
#ifdef WIN32
	LeaveCriticalSection(&section_);
#else
    int  rc = pthread_mutex_unlock(&section_);
//    assert( rc != EINVAL );
//    assert( rc != EPERM );
    assert( rc == 0 );
#endif
}
