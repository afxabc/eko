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

	void updatePoll(const PollerFdPtr& fdptr);
	void removePoll(const PollerFdPtr& fdptr);

	UInt32 runInLoop(const Functor& func, MicroSecond delay = 0)
	{ return loop_.runInLoop(func, delay); }

	bool cancel(UInt32 sequence)
	{ return loop_.cancel(sequence); }

	void quitLoop()
	{ loop_.quitLoop(); }

	void loop()
	{ loop_.loop(); }

	bool loopInThread(Thread::ThreadPriority pri = Thread::THREAD_PRI_NONE)
	{ return loop_.loopInThread(pri); }

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
#ifdef WIN32
	typedef std::vector<WSAEVENT> PollEventVec;
	PollEventVec pollEvents_;
#endif

	typedef std::map<FD, PollerFdPtr> PollerFdMap;
	PollerFdMap fdptrs_;

	SigPtr sigPtr_;
	bool sigMark_;			//action before sigPtr_ open

};

#endif
