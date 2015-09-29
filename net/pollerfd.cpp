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

void PollerFd::handleEvent(short revents)
{
	revents_ = revents;

	if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if (closeCallback_) closeCallback_();
		return;
	}

	if (revents_ & POLLNVAL)
	{
		LOGW("PollerFd::handleEvent() POLLNVAL");
	}

	if (revents_ & (POLLERR | POLLNVAL))
	{
		if (errorCallback_) errorCallback_();
		return;
	}
 
	if (revents_ & (POLLIN | POLLPRI))			// | POLLRDHUP))
	{
		if (readCallback_) readCallback_();
	}

	if (revents_ & POLLOUT)
	{
		if (writeCallback_) writeCallback_();
	}
}

void PollerFd::update()
{
	loop_->updatePoll(shared_from_this());
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
	events_ = 0;
	revents_ = 0;
	index_ = -1;

	sigClose_.on();
}
