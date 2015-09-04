#ifndef NET_POLLER_H_
#define NET_POLLER_H_

#include "pollerfd.h"
#include "base/functorloop.h"
#include <map>
#include <vector>

class PollerLoop : public FunctorLoop
{
public:
	PollerLoop();
	~PollerLoop();

	void updatePoll(const PollerFdPtr& fd);
	void removePoll(const PollerFdPtr& fd);

private:
	void pollLoop();
	void pollWakeup();

private:
	Mutex mutex_;

	typedef std::vector<pollfd> PollFdVec;
	PollFdVec pollfds_;

	typedef std::map<FD, PollerFdPtr> PollerFdMap;
	PollerFdMap fdptrs_;

};

#endif