#include "functorloop.h"

FunctorLoop::FunctorLoop() : threadId_(0), run_(false)
{
	loopFun_ = boost::bind(&FunctorLoop::defaultLoop, this);
	waitFun_ = boost::bind(&FunctorLoop::defaultWait, this, _1);
	wakeupFun_ = boost::bind(&FunctorLoop::defaultWakeup, this);
}

FunctorLoop::~FunctorLoop()
{
	quitLoop();
}

void FunctorLoop::defaultLoop()
{
	assert(!run_ && threadId_== 0);
	threadId_ = Thread::self();

	run_ = true;
	signal_.off();
	queue_.clear();

	static const int TM = 5000;
	MicroSecond delay(TM);

	while(run_)
	{
		waitFun_(delay);
		delay = queue_.run();
		if (delay == 0)
			delay = TM;
	}
}

void FunctorLoop::defaultWait(MicroSecond delay)
{
	signal_.wait(delay);
}

void FunctorLoop::defaultWakeup()
{
	signal_.on();
}

UInt32 FunctorLoop::runInLoop(const Functor& func, MicroSecond delay)
{ 
	UInt32 ret = queue_.post(func, delay);
	wakeupFun_();
	return ret;
}

void FunctorLoop::quitLoop()
{
	run_ = false;
	wakeupFun_();
	thread_.stop();
	queue_.clear();
}
