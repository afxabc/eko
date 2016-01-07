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
	, timeOut_(5000)
{
	sock_init();
	fdptr_->setReadCallback(boost::bind(&TcpClient::handleFdRead, this));
	fdptr_->setWriteCallback(boost::bind(&TcpClient::handleFdWrite, this));
	fdptr_->setCloseCallback(boost::bind(&TcpClient::handleConnectResult, this, false, false));
	fdptr_->setErrorCallback(boost::bind(&TcpClient::handleConnectResult, this, false, false));
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
	, timeOut_(5000)
{
	sock_init();
    fd_nonblock(fd, 1);
	*fdptr_ = fd;
	fdptr_->pollRead();
	fdptr_->setReadCallback(boost::bind(&TcpClient::handleFdRead, this));
	fdptr_->setWriteCallback(boost::bind(&TcpClient::handleFdWrite, this));
	fdptr_->setCloseCallback(boost::bind(&TcpClient::handleConnectResult, this, false, false));
	fdptr_->setErrorCallback(boost::bind(&TcpClient::handleConnectResult, this, false, false));
}

TcpClient::~TcpClient(void)
{
	close();
}

bool TcpClient::open(InetAddress peer, UInt32 timeout)
{
	if (isOpen() || fdptr_->isValid())
		return false;

	FD fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( fd == INVALID_FD )
	{
		LOGE("%s socket error.", __FILE__);
		return false;
	}

	isOpen_ = true;
	timeOut_ = timeout;
	conn_ = NONE;
	peer_  = peer;
#ifdef WIN32
	if (peer.getIPInt() == 0)
		peer_ = InetAddress("127.0.0.1", peer.getPort());
#endif

	if (!loop_->isInLoopThread())
		loop_->runInLoop(boost::bind(&TcpClient::openInLoop, this, fd));
	else openInLoop(fd);

	return true;
}

bool TcpClient::open(const char* addr, UInt16 port, UInt32 timeout)
{
	return open(InetAddress(addr, port), timeout);
}

void TcpClient::openInLoop(FD fd)
{
	assert(loop_->isInLoopThread());
	assert(!fdptr_->isValid());

	*fdptr_ = fd;
	fdptr_->pollRead();

    fd_nonblock(fd, 1);
	tryConnect();
}

void TcpClient::close()
{
	if (!isOpen())
		return;
	isOpen_ = false;

	if (!loop_->isInLoopThread())
	{
		loop_->runInLoop(boost::bind(&TcpClient::closeInLoop, this));
		fdptr_->waitForClose();
	}
	else closeInLoop();
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

	struct linger so_linger;
	so_linger.l_onoff = 0;
	so_linger.l_linger = 0;
	setsockopt(*fdptr_, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof so_linger);
	
	shutdown(*fdptr_, SD_RECEIVE);
	shutdown(*fdptr_, SD_SEND);

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

	if (!conn)
		close();

	conn_ = conn?CONNECTED:NONE;
	if (cbConnect_)
		cbConnect_(peer_, conn);

	if (conn_)
	{
        fdptr_->pollRead();
        sendBuffer();
	}

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
			fdptr_->pollConncet();
			connFuncID_ = loop_->runInLoop(boost::bind(&TcpClient::handleConnectResult, this, false, true), timeOut_);
		}
	}
	else
	{
		handleConnectResult((ret == 0));
	}

}

int TcpClient::sendData(const char* data, int len)
{
	if (!isOpen() || len <= 0 || data == NULL)
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
	else fdptr_->pollWrite(true, true);	//for win32, force to write

	return ret;
}

void TcpClient::handleFdRead()
{
	if (!isOpen())
		return;

	assert(loop_->isInLoopThread());

	char buf[4096];
	int len = recv(*fdptr_, buf, 4096, 0);

	if (len <= 0)
	{
		handleConnectResult(false);
		return;
	}

	if (cbRead_)
		cbRead_(buf, len);
}

void TcpClient::sendBuffer()
{
	assert(loop_->isInLoopThread());
	//try send
	int slen = 0;
	{
		Lock lock(mutexSend_);

		int buffSize = sendBuff_.readableBytes();

		if (buffSize > 0)
		{
			slen = sock_send(*fdptr_, sendBuff_);

			if (slen < 0 )
			{
				handleConnectResult(false);
				return;
			}

			if (slen > 0)
				sendBuff_.eraseFront(slen);

			if (sendBuff_.readableBytes() == 0)
				fdptr_->pollWrite(false);
			else fdptr_->pollWrite();

		}
	}

	if (slen > 0 && cbSend_)
		cbSend_(slen);
}

void TcpClient::handleFdWrite()
{
	if (!isOpen())
		return;

	assert(loop_->isInLoopThread());

	if (conn_ == CONNECTING)
		handleConnectResult(true);
	else if (conn_ == CONNECTED)
		sendBuffer();

}
