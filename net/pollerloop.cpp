#include "pollerloop.h"

PollerLoop::PollerLoop()
{
	setLoop(boost::bind(&PollerLoop::pollLoop, this));
	setWakeup(boost::bind(&PollerLoop::pollWakeup, this));
}

PollerLoop::~PollerLoop()
{
}

void PollerLoop::updatePoll(const PollerFdPtr& fdptr)
{
	if (!isInLoopThread())
	{
		runInLoop(boost::bind(&PollerLoop::updatePoll, this, fdptr));
		return;
	}

	BOOST_AUTO(it, fdptrs_.find(fdptr->fd()));

	if (fdptr->index() < 0)		//new one
	{
		assert(it == fdptrs_.end());

		struct pollfd pfd;
		pfd.fd = fdptr->fd();
		pfd.events = fdptr->events();
		pfd.revents = 0;
		pollfds_.push_back(pfd);

		int idx = static_cast<int>(pollfds_.size())-1;
		fdptr->set_index(idx);
		fdptrs_[pfd.fd] = fdptr;
	}
	else
	{
		assert(it != fdptrs_.end());
		assert(fdptr == it->second);
    
		int idx = fdptr->index();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

		struct pollfd& pfd = pollfds_[idx];
		assert(pfd.fd == fdptr->fd());// || pfd.fd == -channel->fd()-1);		
		pfd.events = fdptr->events();
		pfd.revents = 0;
	}
}

void PollerLoop::removePoll(const PollerFdPtr& fdptr)
{
	if (!isInLoopThread())
	{
		runInLoop(boost::bind(&PollerLoop::removePoll, this, fdptr));
		return;
	}

	BOOST_AUTO(it, fdptrs_.find(fdptr->fd()));
	
	assert(it != fdptrs_.end());
	assert(fdptr == it->second);

	int idx = fdptr->index();
	assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

	fdptrs_.erase(it);

	if (idx != pollfds_.size()-1)
	{
		int fdEnd = pollfds_.back().fd;
		std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
		fdptrs_[fdEnd]->set_index(idx);
	}
	pollfds_.pop_back();
}

void PollerLoop::pollLoop()
{
	assert(!run_ && threadId_== 0);
	threadId_ = Thread::self();

	run_ = true;
	queue_.clear();

	static const int TM = 5000;
	MicroSecond delay(TM);

	while(run_)
	{
		int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), delay);
		int savedErrno = ::error_n();
  
		if (numEvents == 0)
		{
			LOGI("poll nothing, check for timeout fuction");
			delay = queue_.run();
			if (delay == 0)
				delay = TM;
			continue;
		}
		else if (numEvents < 0)
		{
			LOGE("poll error: %d !!", savedErrno);
			continue;
		}

		//numEvents > 0
		Timestamp now;
		BOOST_AUTO(pfd, pollfds_.begin());
		for (; pfd != pollfds_.end() && numEvents > 0; ++pfd)
		{
			if (pfd->revents > 0)
			{
				--numEvents;
				BOOST_AUTO(it, fdptrs_.find(pfd->fd));
				assert(it != fdptrs_.end());
				it->second->handleEvent(now, pfd->revents);
			}
		}
	}
}

void PollerLoop::pollWakeup()
{
}
