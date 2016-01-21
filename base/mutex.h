#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include "common.h"

class Lockable
{
public:
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

////////////////////////////////////////////////////

class Lock
{
public:
	Lock(Lockable& l) : lock_(l) 
	{
		lock_.lock();
	}

	~Lock(void)
	{
		lock_.unlock();
	}

private:
	Lockable& lock_;
};

////////////////////////////////////////////////////

class Mutex : public Lockable, boost::noncopyable
{
#ifndef WIN32
	typedef pthread_mutex_t CRITICAL_SECTION;
#endif

public:
	Mutex();
	virtual ~Mutex();
	virtual void lock();
	virtual void unlock();

private:
	CRITICAL_SECTION section_;
	int count_;
};

#endif
