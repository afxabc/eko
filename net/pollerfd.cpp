#include "base/log.h"
#include "pollerfd.h"
#include "pollerloop.h"

PollerFd::PollerFd(FD fd, PollerLoop* loop) 
	: fd_(fd)
	, loop_(loop)
	, index_(-1)
	, events_(0)
	, revents_(0)
{
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

void PollerFd::update()
{
	loop_->updatePoll(shared_from_this());
}
