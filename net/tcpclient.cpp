#include "tcpclient.h"
#include "pollerloop.h"

TcpClient::TcpClient(PollerLoop* loop, int sendBuffInitSize)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
	, autoConnect_(true)
	, sendBuff_(sendBuffInitSize)
	, conn_(NONE)
	, isOpen_(false)
	, connFuncID_(0)
{
	fdptr_->setReadCallback(boost::bind(&TcpClient::handleFdRead, this, _1));
	fdptr_->setWriteCallback(boost::bind(&TcpClient::handleFdWrite, this));
}

TcpClient::TcpClient(FD fd, PollerLoop* loop, InetAddress peer, int sendBuffInitSize)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
	, autoConnect_(false)
	, conn_(CONNECTED)					//for server, accept
	, sendBuff_(sendBuffInitSize)
	, peer_(peer)
	, isOpen_(true)
	, connFuncID_(0)
{
    fd_nonblock(fd, 1);
	*fdptr_ = fd;
	fdptr_->enableReading(true);
	fdptr_->setReadCallback(boost::bind(&TcpClient::handleFdRead, this, _1));
	fdptr_->setWriteCallback(boost::bind(&TcpClient::handleFdWrite, this));
}

TcpClient::~TcpClient(void)
{
	close();
}

bool TcpClient::open(InetAddress peer)
{
	if (!loop_->isInLoopThread())
	{
		if (isOpen())
			return false;
		isOpen_ = true;
		loop_->runInLoop(boost::bind(&TcpClient::open, this, peer));
		return true;
	}

	isOpen_ = true;
	if (fdptr_->isValid())
		return false;

	peer_  = peer;

	FD fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( fd == INVALID_FD )
	{
		LOGE("%s socket error.", __FILE__);
		isOpen_ = false;
		return false;
	}

    fd_nonblock(fd, 1);
	*fdptr_ = fd;
	fdptr_->enableReading();

	conn_ = NONE;
	tryConnect();

	return true;
}

bool TcpClient::open(const char* addr, UInt16 port)
{
	return open(InetAddress(addr, port));
}

void TcpClient::close()
{
	if (!isOpen())
		return;

	isOpen_ = false;
	loop_->runInLoop(boost::bind(&TcpClient::closeInLoop, this));

	if (!loop_->isInLoopThread())
		fdptr_->waitForClose();

}

void TcpClient::closeInLoop()
{
	assert(loop_->isInLoopThread());

	isOpen_ = false;

	if (connFuncID_)
		loop_->cancel(connFuncID_);
	connFuncID_ = 0;

	conn_ = NONE;
	{
		Lock lock(mutexSend_);
		sendBuff_.erase();
	}

	fdptr_->close();
}

void TcpClient::handleConnectResult(bool conn, bool tmOut)
{	
	assert(loop_->isInLoopThread());

	if (connFuncID_ && !tmOut)
		loop_->cancel(connFuncID_);
	connFuncID_ = 0;

	if (tmOut && conn_==CONNECTED)
		return;
		
	if (cbConnect_)
		cbConnect_(peer_, conn);
		
	conn_ = conn?CONNECTED:NONE;

	if (!conn)
		close();
}

void TcpClient::tryConnect()
{
	if (!loop_->isInLoopThread())
	{
		loop_->runInLoop(boost::bind(&TcpClient::tryConnect, this));
		return;
	}

	if (!isOpen() || !fdptr_->isValid() || !autoConnect_ || conn_==CONNECTED)
		return;

	int ret = sock_connect(*fdptr_, peer_);
	if (ret == 1)		//try
	{
		if (connFuncID_ == 0)
		{
			conn_ = CONNECTING;
			fdptr_->enableWriting();
			connFuncID_ = loop_->runInLoop(boost::bind(&TcpClient::handleConnectResult, this, false, true), CONNECT_TIMEOUT);
		}
	}
	else
	{
		handleConnectResult((ret == 0));
	}

}

int TcpClient::sendData(const char* data, int len)
{
	if (!isOpen())
		return -1;

	int ret = len;
	{
		Lock lock(mutexSend_);
		if (sendBuff_.readableBytes() < MAX_SENDBUFF_SIZE)
			sendBuff_.pushBack(data, len, true);
		else ret = 0;
	}

	if (conn_ != CONNECTED)
		tryConnect();

	fdptr_->enableWriting();

	return ret;
}

void TcpClient::handleFdRead(Timestamp receiveTime)
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

	char buf[4096];
	int len = recv(*fdptr_, buf, 4096, 0);
	if (len <= 0)
	{
		handleConnectResult(false);
		return;
	}

	if (cbRead_)
		cbRead_(receiveTime, Buffer(buf, len));
}

void TcpClient::handleFdWrite()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

	if (conn_ == CONNECTING)
	{
		handleConnectResult(true);
	}

	if (conn_ != CONNECTED)
		return;

	//try send
	int slen = 0;
	{
		Lock lock(mutexSend_);
		slen = send(*fdptr_, sendBuff_.beginRead(), sendBuff_.readableBytes(), 0);
		if (slen > 0)
			sendBuff_.eraseFront(slen);

		if (sendBuff_.readableBytes() == 0)
			fdptr_->disableWriting();
	}

	if (cbSend_)
		cbSend_(slen);
}
