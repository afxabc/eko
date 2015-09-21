#ifndef NET_POLLERLOOP_H_
#define NET_POLLERLOOP_H_

#include "pollerfd.h"
#include "pollersig.h"
#include "base/functorloop.h"
#include <map>
#include <vector>

class PollerLoop : public boost::noncopyable
{
public:
	PollerLoop();
	~PollerLoop();

	void updatePoll(const PollerFdPtr& fd);
	void removePoll(const PollerFdPtr& fd);

	UInt32 runInLoop(const Functor& func, MicroSecond delay = 0)
	{ return loop_.runInLoop(func, delay); }

	bool cancel(UInt32 sequence)
	{ return loop_.cancel(sequence); }
	
	void quitLoop()		
	{ loop_.quitLoop(); }

	void loop() 
	{ loop_.loop(); }	

	bool loopInThread()
	{ return loop_.loopInThread(); }

	bool isRun()
	{ return loop_.isRun(); }

	bool isInLoopThread()
	{ return loop_.isInLoopThread(); }

	int getPending()
	{ return loop_.getPending(); }


private:
	void pollLoop();
	void pollWakeup();

private:
	Mutex mutex_;

	FunctorLoop loop_;

	typedef std::vector<pollfd> PollFdVec;
	PollFdVec pollfds_;
	bool pollfdsChanged_;

	typedef std::map<FD, PollerFdPtr> PollerFdMap;
	PollerFdMap fdptrs_;

	SigPtr sigPtr_;
	bool sigMark_;			//action before sigPtr_ open

};

#endif