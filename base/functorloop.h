#ifndef _BASE_FUNCTOR_LOOP_H__
#define _BASE_FUNCTOR_LOOP_H__ 

#include "thread.h"
#include "signal.h"
#include "functorqueue.h"


typedef boost::function<void(MicroSecond)> WaitFunctor;
class FunctorLoop : public boost::noncopyable
{
public:
	FunctorLoop();
	virtual ~FunctorLoop();

	void loop() 
	{ loopFun_(); }		//进入loop

	bool loopInThread()	//在新的线程loop
	{ return thread_.start(boost::bind(&FunctorLoop::loop, this)); }

	void quitLoop();		//退出loop

	bool isInLoopThread()
	{ return (threadId_ == Thread::self()); }

	UInt32 runInLoop(const Functor& func, MicroSecond delay = 0);

	bool cancel(UInt32 sequence)
	{ return queue_.cancel(sequence); }

	void setLoop(Functor fun)
	{ loopFun_ = fun; }

	void setWait(WaitFunctor fun)
	{ waitFun_ = fun; }

	void setWakeup(Functor fun)
	{ wakeupFun_ = fun; }

private:
	void defaultLoop();
	void defaultWait(MicroSecond ms);
	void defaultWakeup();

protected:
	Functor loopFun_;
	WaitFunctor waitFun_;
	Functor wakeupFun_;

	FunctorQueue queue_;
	Thread thread_;
	UInt32 threadId_;
	bool run_;
	Signal signal_;
};

#endif
