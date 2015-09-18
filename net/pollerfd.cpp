#include "base/log.h"
#include "pollerfd.h"
#include "pollerloop.h"

PollerFd::PollerFd(PollerLoop* loop) 
	: fd_(INVALID_FD)
	, loop_(loop)
	, index_(-1)
	, events_(0)
	, revents_(0)
{
	assert(loop != NULL);
	sigClose_.off();
}

void PollerFd::handleEvent(Timestamp receiveTime, short revents)
{
	revents_ = revents;

	if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if (closeCallback_) closeCallback_();
	}

	if (revents_ & POLLNVAL)
	{
		LOGW("PollerFd::handleEvent() POLLNVAL");
	}

	if (revents_ & (POLLERR | POLLNVAL))
	{
		if (errorCallback_) errorCallback_();
	}
 
	if (revents_ & (POLLIN | POLLPRI))			// | POLLRDHUP))
	{
		if (readCallback_) readCallback_(receiveTime);
	}

	if (revents_ & POLLOUT)
	{
		if (writeCallback_) writeCallback_();
	}
}

void PollerFd::update(bool forceInLoop)
{
	if (forceInLoop)	
		loop_->runInLoop(boost::bind(&PollerLoop::updatePoll, loop_, shared_from_this()));
	else loop_->updatePoll(shared_from_this());
}

void PollerFd::close()
{
	assert(loop_->isInLoopThread());
	
	if (isValid())
	{
		loop_->removePoll(shared_from_this());
		closefd(fd_);
		fd_ = INVALID_FD;
	}

	sigClose_.on();
}
