#ifndef NET_POLLER_TCPCLIENT_H_
#define NET_POLLER_TCPCLIENT_H_

#include "pollerfd.h"
#include "base/buffer.h"
#include "base/signal.h"
#include "base/timestamp.h"

class PollerLoop;

typedef boost::function<void(char*, int)> TcpReadCallback;
typedef boost::function<void(InetAddress, bool)> TcpConnectCallback;
typedef boost::function<void(int)> TcpSendCallback;

class TcpClient
{
public:
	TcpClient(PollerLoop* loop, int sendBuffInitSize = 4096);
	TcpClient(FD fd, PollerLoop* loop, InetAddress peer, int sendBuffInitSize = 4096);	//for server, accept
	~TcpClient(void);

	bool open(InetAddress peer, UInt32 timeout = 5000);
	bool open(const char* addr, UInt16 port, UInt32 timeout = 5000);
	void close();
	int sendData(const char* data, int len);

	bool isOpen()
	{
		return isOpen_;
	}

	bool isConnected()
	{
		return (conn_ == CONNECTED);
	}

	void setReadCallback(const TcpReadCallback& cb)
	{
		cbRead_ = cb;
	}

	void setSendCallback(const TcpSendCallback& cb)
	{
		cbSend_ = cb;
	}

	void setConnectCallback(const TcpConnectCallback& cb)
	{
		cbConnect_ = cb;
	}

	InetAddress peer() const
	{ return peer_; }

	int getSendPending() const
	{ return (int)sendBuff_.readableBytes(); }

	FD fd() const
	{ return fdptr_->fd(); }

private:
	void tryConnect();
	void sendBuffer();
	void handleFdRead();
	void handleFdWrite();
	void closeInLoop();
	void handleConnectResult(bool conn, bool tmOut = false);

private:
	PollerLoop* loop_;
	PollerFdPtr fdptr_;
	InetAddress peer_;
	TcpReadCallback cbRead_;
	TcpSendCallback cbSend_;
	TcpConnectCallback cbConnect_;

	static const int MAX_SENDBUFF_SIZE = 1*1024*1024;
	Mutex mutexSend_;
	Buffer sendBuff_;

	bool autoConnect_;
    enum ConnectStatus { NONE = 0, CONNECTING, CONNECTED };
	ConnectStatus conn_;

	bool isOpen_;

	UInt32 timeOut_;
	UInt32 connFuncID_;
};

typedef boost::shared_ptr<TcpClient> TcpClientPtr;

#endif
