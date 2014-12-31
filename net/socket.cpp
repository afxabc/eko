#include "base/log.h"
#include "socket.h"

Socket::Socket(SOCKET socket) 
	: socket_(socket),
	  index_(-1),
	  events_(0),
	  revents_(0)
{
}

void Socket::handleEvent(Timestamp receiveTime)
{
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if (closeCallback_) closeCallback_();
	}

	if (revents_ & POLLNVAL)
	{
		LOGW("Socket::handleEvent() POLLNVAL");
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
