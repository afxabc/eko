#ifndef NET_POLLER_TCPSERVER_H_
#define NET_POLLER_TCPSERVER_H_

#include "pollerfd.h"
#include "base/buffer.h"
#include "base/signal.h"
#include "base/timestamp.h"

class PollerLoop;

#include "tcpclient.h"
typedef boost::function<void(TcpClientPtr)> TcpAcceptCallback;

class TcpServer
{
public:
	TcpServer(PollerLoop* loop);
	~TcpServer(void);
	
	bool open(InetAddress local);
	void close();

	bool isOpen()
	{
		return isOpen_;
	}

	void setAcceptCallback(const TcpAcceptCallback& cb)
	{ 
		cbAccept_ = cb; 
	}

private:
	void handleFdRead();

private:
	PollerLoop* loop_;
	PollerFdPtr fdptr_;
	InetAddress local_;

	TcpAcceptCallback cbAccept_;

	bool isOpen_;
};

typedef boost::shared_ptr<TcpServer> TcpServerPtr;

#endif
