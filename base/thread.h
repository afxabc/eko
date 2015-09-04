#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_

#include "common.h"
#include "timestamp.h"

class Thread : boost::noncopyable
{
public:
#ifdef WIN32
      typedef unsigned int THREAD_ID;
#else
      typedef int HANDLE;
      typedef pthread_t THREAD_ID;
#endif

	Thread(void);
	Thread(const Functor& func);
	~Thread(void);
	
public:
	inline static void sleep(MicroSecond ms)
	{
#ifdef WIN32
		Sleep(ms);
#else
		usleep(ms*1000);
#endif
	}

	inline static THREAD_ID self()
	{
#ifdef WIN32
		return (THREAD_ID)GetCurrentThreadId();
#else
		return (THREAD_ID)pthread_self();
#endif
	}

public:
	bool start(const Functor& func);
	void stop();

	bool started() { return run_; }
	THREAD_ID id() { return thread_id_; }
	bool isInThread() { return (thread_id_ == self()); }

private:
#ifdef WIN32
	static UINT WINAPI _threadFunc(void* p);
#else
	static void* _threadFunc(void* p);
#endif

protected:
	bool run_;
      HANDLE handle_;
      THREAD_ID thread_id_;

	  Functor func_;
};

#endif
