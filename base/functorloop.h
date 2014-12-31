#ifndef _BASE_FUNCTOR_LOOP_H__
#define _BASE_FUNCTOR_LOOP_H__ 

#include "thread.h"
#include "signal.h"
#include "functorqueue.h"

class FunctorLoop : public boost::noncopyable
{
public:
	FunctorLoop();
	~FunctorLoop();

	void loop();		//进入loop
	void quit();		//退出loop

	void loopInThread()	//在新的线程loop
	{ thread_.start(boost::bind(&FunctorLoop::loop, this)); }

	UInt32 run(const Functor& func, MicroSecond delay = 0);

	bool cancel(UInt32 sequence)
	{ return queue_.cancel(sequence); }

private:
	FunctorQueue queue_;
	Thread thread_;
	UInt32 threadId_;
	bool run_;
	Signal signal_;
};

#endif
