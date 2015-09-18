#ifndef NET_POLLERFD_H__
#define NET_POLLERFD_H__

#include "inetaddress.h"
#include "base/timestamp.h"
#include "base/signal.h"

class PollerLoop;
class PollerFd : public boost::enable_shared_from_this<PollerFd>
{
	//callback
	typedef boost::function<void()> EventCallback;
	typedef boost::function<void(Timestamp)> ReadEventCallback;

public:
	PollerFd(PollerLoop* loop);

	operator FD() const { return fd_; }
	FD operator=(FD fd)
	{
		fd_ = fd; 
		if (isValid())
			sigClose_.off();
		else sigClose_.on();
		return fd_;
	}

	bool isValid()
	{
		return (fd_ != INVALID_FD);
	}

	//for poller
	FD fd() const { return fd_; }

	int index() { return index_; }
	void set_index(int idx) { index_ = idx; }

	void set_revents(short revt) { revents_ = revt; }

	short events() const { return events_; }
	bool isWriting() const { return ((events_ & POLLOUT) != 0); }

	void enableReading(bool forceInLoop = false) { events_ |= (POLLIN | POLLPRI); update(forceInLoop); }
	void disableReading(bool forceInLoop = false) { events_ &= ~(POLLIN | POLLPRI); update(forceInLoop); }
	void enableWriting(bool forceInLoop = false) { events_ |= POLLOUT; update(forceInLoop); }
	void disableWriting(bool forceInLoop = false) { events_ &= ~POLLOUT; update(forceInLoop); }
	void disableAll(bool forceInLoop = false) { events_ = 0; update(forceInLoop); }

	
	//callback
	void setReadCallback(const ReadEventCallback& cb)
	{ readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{ writeCallback_ = cb; }
	void setCloseCallback(const EventCallback& cb)
	{ closeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{ errorCallback_ = cb; }

	void handleEvent(Timestamp receiveTime, short revents);

	void close();
	void waitForClose()
	{ sigClose_.wait(); }

private:
	void update(bool forceInLoop);

private:
	FD fd_;
	Signal sigClose_;

	//for poller
	PollerLoop* loop_;
	mutable int index_;
	short events_;
	short revents_;

	//callback
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};

typedef boost::shared_ptr<PollerFd> PollerFdPtr;

#endif
