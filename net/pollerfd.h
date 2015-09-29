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

	short events() const { return events_; }

	void pollRead(bool enable = true)
	{
		if (enable)
			events_ |= (POLLIN | POLLPRI);
		else events_ &= ~(POLLIN | POLLPRI);
		update();
	}

	void pollWrite(bool enable = true)
	{
		if (enable)
			events_ |= POLLOUT;
		else events_ &= ~POLLOUT;
		update();
	}

	//only for win32, force to write,
	void triggerWrite()
	{
#ifdef WIN32
		events_ |= POLLWRITE;
		update();
		events_ &= ~POLLWRITE;
#endif
	}

	void pollConncet(bool enable = true)
	{
#ifdef WIN32
		if (enable)
			events_ |= POLLCONN;
		else events_ &= ~POLLCONN;
		update();
#else
		pollWrite(enable);
#endif
	}

	void pollNone()
	{
		events_ = 0;
		update();
	}


	//callback
	void setReadCallback(const EventCallback& cb)
	{ readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{ writeCallback_ = cb; }
	void setCloseCallback(const EventCallback& cb)
	{ closeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{ errorCallback_ = cb; }

	void handleEvent(short revents);

	void close();
	void waitForClose()
	{ sigClose_.wait(); }

private:
	void update();

private:
	FD fd_;
	Signal sigClose_;

	//for poller
	PollerLoop* loop_;
	mutable int index_;
	short events_;
	short revents_;

	//callback
	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};

typedef boost::shared_ptr<PollerFd> PollerFdPtr;

#endif
