#ifndef NET_SOCKET_H__
#define NET_SOCKET_H__

#include "inetaddress.h"
#include "base/timestamp.h"

class Socket
{
	//callback
	typedef boost::function<void()> EventCallback;
	typedef boost::function<void(Timestamp)> ReadEventCallback;

public:
	Socket(SOCKET socket = INVALID_SOCKET);

	operator SOCKET() const { return socket_; }
	SOCKET operator=(SOCKET socket)
	{
		socket_ = socket; 
		return socket_;
	}

	//for poller
	int index() { return index_; }
	void set_index(int idx) { index_ = idx; }

	void set_revents(short revt) { revents_ = revt; }

	short events() const { return events_; }
	void enableReading() { events_ |= (POLLIN | POLLPRI); update(); }
	void disableReading() { events_ &= ~(POLLIN | POLLPRI); update(); }
	void enableWriting() { events_ |= POLLOUT; update(); }
	void disableWriting() { events_ &= ~POLLOUT; update(); }
	void disableAll() { events_ = 0; update(); }
	bool isWriting() const { return ((events_ & POLLOUT) != 0); }
	
	//callback
	void setReadCallback(const ReadEventCallback& cb)
	{ readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{ writeCallback_ = cb; }
	void setCloseCallback(const EventCallback& cb)
	{ closeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{ errorCallback_ = cb; }

	void handleEvent(Timestamp receiveTime);

private:
	void update() {}

private:
	SOCKET socket_;
	InetAddress addr_;

	//for poller
	int index_;
	short events_;
	short revents_;

	//callback
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};

typedef boost::shared_ptr<Socket> SocketPtr;

#endif
