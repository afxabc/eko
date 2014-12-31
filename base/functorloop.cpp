#include "functorloop.h"

FunctorLoop::FunctorLoop() : threadId_(0), run_(false)
{
}

FunctorLoop::~FunctorLoop()
{
	quit();
}

void FunctorLoop::loop()
{
	assert(!run_ && threadId_==0);
	threadId_ = Thread::self();

	run_ = true;
	signal_.off();
	queue_.clear();

	static const int TM = 5000;
	MicroSecond delay(TM);

	while(run_)
	{
		signal_.wait(delay);
		delay = queue_.run();
		if (delay == 0)
			delay = TM;
	}
}

UInt32 FunctorLoop::run(const Functor& func, MicroSecond delay)
{ 
	UInt32 ret = queue_.post(func, delay);
	signal_.on();
	return ret;
}

void FunctorLoop::quit()
{
	run_ = false;
	signal_.on();
	thread_.stop();
	queue_.clear();
}
