#include "pollerloop.h"

PollerLoop::PollerLoop()
	: pollfdsChanged_(false)
	, sigPtr_(new PollerSig(this))
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

	int idx = -1;
	if (fdptr->index() < 0)		//new one
	{
		assert(it == fdptrs_.end());

		struct pollfd pfd;
		pfd.fd = fdptr->fd();
		pfd.events = 0;
		pfd.revents = 0;
#ifdef WIN32
		pfd.wevent = WSACreateEvent();
#endif
		pollfds_.push_back(pfd);

		idx = static_cast<int>(pollfds_.size())-1;
		fdptr->set_index(idx);
		fdptrs_[pfd.fd] = fdptr;

		pollfdsChanged_ = true;

		LOGD("updatePoll : [%d]=%d", fdptr->index(), fdptr->fd());
 //       LOGD("updatePoll : fdptr(%d) ref=%d", fdptr->fd(), fdptr.use_count());
	}
	else
	{
		assert(it != fdptrs_.end());
		assert(fdptr == it->second);

		idx = fdptr->index();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

	}

	//set
	if (idx >= 0)
	{
		struct pollfd& pfd = pollfds_[idx];
		assert(pfd.fd == fdptr->fd());// || pfd.fd == -channel->fd()-1);
		pfd.events = fdptr->events();
		pfd.revents = 0;

#ifdef WIN32
		long lEvents = 0;
		lEvents |= (pfd.events&POLLIN)?(FD_READ|FD_ACCEPT|FD_CLOSE):0;
		lEvents |= (pfd.events&POLLOUT)?(FD_WRITE):0;
		lEvents |= (pfd.events&POLLCONN)?(FD_CONNECT):0;
		if (SIG_FD != pfd.fd)
		{
			int ret = WSAEventSelect(pfd.fd, pfd.wevent, lEvents);
			assert(ret != SOCKET_ERROR);

			if (pfd.events & POLLWRITE)
			{
				pfd.revents |= POLLOUT;
				WSASetEvent(pfd.wevent);
			}
		}

#endif
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

	LOGD("removePoll : [%d]=%d", fdptr->index(), fdptr->fd());

	pollfds_[idx].events = 0;
	pollfds_[idx].revents = 0;
#ifdef WIN32
	WSACloseEvent(pollfds_[idx].wevent);
#endif

	fdptrs_.erase(it);

	if (idx != pollfds_.size()-1)
	{
		int fdEnd = pollfds_.back().fd;
		std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
		fdptrs_[fdEnd]->set_index(idx);
	}
	pollfds_.pop_back();

	fdptr->set_index(-1);

	pollfdsChanged_ = true;

//	LOGD("removePoll : fdptr(%d) ref=%d", fdptr->fd(), fdptr.use_count());
}

void PollerLoop::pollLoop()
{
	LOGD("PollerLoop enter.");

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
			BOOST_AUTO(pfd, pollfds_.begin());
			for (int i=0; i<pollfds_.size(); ++i)
			{
				if (pollfds_[i].revents <= 0)
					continue;

				if (!pollfdsChanged_)
				{

					if (pollfds_[i].fd == sigPtr_->fd())
						queueRun = true;

					//--numEvents;
					BOOST_AUTO(it, fdptrs_.find(pollfds_[i].fd));
					assert(it != fdptrs_.end());
					short revents = pollfds_[i].revents;
					pollfds_[i].revents = 0;
					it->second->handleEvent(revents);

		//			WSAResetEvent(pollfds_[i].wevent);
				}
#ifdef WIN32
				else if (pollfds_[i].fd != sigPtr_->fd())
					WSASetEvent(pollfds_[i].wevent);
#endif

			}
		}

		pollfdsChanged_ = false;

		if (queueRun || loop_.getActPending() > 0)
		{
			delay = loop_.runQueue();
			if (delay == 0)
				delay = TM;
		}
	}

	sigPtr_->close();
	pollfds_.clear();
	fdptrs_.clear();

	LOGD("PollerLoop quit.");
}

void PollerLoop::pollWakeup()
{
	if (sigPtr_ && sigPtr_->isOpen())
	{
#ifdef WIN32
		int idx = sigPtr_->index();
		struct pollfd& pfd = pollfds_[idx];
		WSASetEvent(pfd.wevent);
#else
		sigPtr_->signal();
#endif
	}
	else sigMark_ = true;
}
