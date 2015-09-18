#include "pollerloop.h"

PollerLoop::PollerLoop()
	: sigPtr_(new PollerSig(this))
	, sigMark_(false)
{
	loop_.setLoop(boost::bind(&PollerLoop::pollLoop, this));
	loop_.setWakeup(boost::bind(&PollerLoop::pollWakeup, this));
}

PollerLoop::~PollerLoop()
{
	loop_.quitLoop();
}

void PollerLoop::updatePoll(const PollerFdPtr& fdptr)
{		
//	assert(fdptr->fd() != INVALID_FD);

	if (!isInLoopThread())
	{
		runInLoop(boost::bind(&PollerLoop::updatePoll, this, fdptr));
		return;
	}

	if (fdptr->fd() == INVALID_FD)
		return;

	BOOST_AUTO(it, fdptrs_.find(fdptr->fd()));

	if (it == fdptrs_.end())
	{
		fdptr->set_index(-1);
	}

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
	
		LOGI("updatePoll : [%d]=%d", fdptr->index(), fdptr->fd());
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
	assert(fdptr->fd() != INVALID_FD);

	if (!isInLoopThread())
	{
		runInLoop(boost::bind(&PollerLoop::removePoll, this, fdptr));
		return;
	}

	if (fdptr->fd() == INVALID_FD)
		return;

	BOOST_AUTO(it, fdptrs_.find(fdptr->fd()));
	
	assert(it != fdptrs_.end());
	assert(fdptr == it->second);

	int idx = fdptr->index();
	assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

	LOGI("removePoll : [%d]=%d", fdptr->index(), fdptr->fd());

	fdptrs_.erase(it);

	if (idx != pollfds_.size()-1)
	{
		int fdEnd = pollfds_.back().fd;
		std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
		fdptrs_[fdEnd]->set_index(idx);
	}
	pollfds_.pop_back();
	
	fdptr->set_index(-1);

}

void PollerLoop::pollLoop()
{
	
	LOGI("PollerLoop enter.");

	static const int TM = 5000;
	MicroSecond delay(TM);

	sigPtr_->open();

	while (loop_.isRun())
	{
		assert(pollfds_.size() > 0);

		int numEvents = 0;
		int savedErrno = 0;
  
		if (!sigMark_)
		{
			numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), delay);
			savedErrno = ::error_n();
		}
		
		sigMark_ = false;

		bool queueRun = false;
		if (numEvents == 0)
		{
//			LOGI("poll nothing, check for timeout fuction");
			queueRun = true;
		}
		else if (numEvents < 0)
		{
			LOGE("poll error: %d !!", savedErrno);
			continue;
		}
		else	//numEvents > 0
		{
			Timestamp now;
			BOOST_AUTO(pfd, pollfds_.begin());
			for (int i=0; i<pollfds_.size() && numEvents > 0; ++i)
			{
				if (pollfds_[i].revents > 0)
				{
					if (pollfds_[i].fd == sigPtr_->fd())
						queueRun = true;

					--numEvents;
					BOOST_AUTO(it, fdptrs_.find(pollfds_[i].fd));
					assert(it != fdptrs_.end());
					it->second->handleEvent(now, pollfds_[i].revents);
					pollfds_[i].revents = 0;
				}
			}
		}

		if (queueRun)
		{
			delay = loop_.runQueue();
			if (delay == 0)
				delay = TM;
		}
	}

	sigPtr_->close();
	pollfds_.clear();
	fdptrs_.clear();
			
	LOGI("PollerLoop quit.");
}

void PollerLoop::pollWakeup()
{
	if (sigPtr_ && sigPtr_->isOpen())
	{
		sigPtr_->signal();
	}
	else sigMark_ = true;
}
