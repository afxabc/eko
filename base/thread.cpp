
#include "common.h"
#include "thread.h"
#include "log.h"

#ifdef WIN32
#include <process.h>
#endif


#ifdef WIN32
UINT WINAPI Thread::_threadFunc(void* p)
#else
void* Thread::_threadFunc(void* p)
#endif
{
   Thread* pth = static_cast < Thread* > (p);
   pth->func_();

#ifdef WIN32
   _endthreadex(0);
#endif

   return 0;
}

//////////////////////////////////////////////////////////////////////

Thread::Thread(void) : run_(false), handle_(NULL), thread_id_(0)
{
}

Thread::Thread(const Functor& func) : run_(false), handle_(NULL), thread_id_(0)
{
	start(func);
}

Thread::~Thread(void)
{
}

bool Thread::start(const Functor& func)
{
	if (run_)
		return false;

	func_ = func;

#ifdef WIN32
	handle_ = (HANDLE)_beginthreadex(NULL, 0, _threadFunc, this, 0, &thread_id_);
	run_ = (handle_ != NULL);
#else
	int retval = pthread_create(&thread_id_, 0, _threadFunc, (void*)this);
	run_ = (retval == 0);
#endif

	return run_;
}
	
void Thread::stop()
{
	if (!run_)
		return;
	
	run_ = false;
#ifdef WIN32
	if (!isInThread())
	{
		WaitForSingleObject(handle_, INFINITE);
	}
	if (handle_ != NULL)
	{
		CloseHandle(handle_);
		handle_ = NULL;
	}
#else
	void* stat;
	if (!isInThread())
	{
		pthread_join( thread_id_ , &stat );
	}
#endif
	thread_id_ = 0;
}
