#include "tcpserver.h"
#include "pollerloop.h"

TcpServer::TcpServer(PollerLoop* loop)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
	, isOpen_(false)
{
	sock_init();
	fdptr_->setReadCallback(boost::bind(&TcpServer::handleFdRead, this));
}

TcpServer::~TcpServer(void)
{
	close();
}

bool TcpServer::open(InetAddress local)
{
	if (!loop_->isInLoopThread())
	{
		if (isOpen())
			return false;
		isOpen_ = true;
		loop_->runInLoop(boost::bind(&TcpServer::open, this, local));
		return true;
	}

	isOpen_ = true;
	if (fdptr_->isValid())
		return false;

	local_  = local;
	assert(!local_.isNull());

	FD fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( fd == INVALID_FD )
	{
		LOGE("%s socket error.", __FILE__);
		isOpen_ = false;
		return false;
	}

    fd_nonblock(fd, 1);

	if (sock_bind(fd, local_)!=0 || listen(fd, 5)<0)
	{
		LOGE("%s socket bind or listen.", __FILE__);
		closefd(fd);
		isOpen_ = false;
		return false;
	}

	*fdptr_ = fd;
	fdptr_->pollRead();

	return true;
}

void TcpServer::close()
{
	if (!loop_->isInLoopThread())
	{
		if (!isOpen())
			return;
		isOpen_ = false;
		loop_->runInLoop(boost::bind(&TcpServer::close, this));
		fdptr_->waitForClose();
		return;
	}

	isOpen_ = false;

	fdptr_->close();
}

void TcpServer::handleFdRead()
{
	assert(loop_->isInLoopThread());

	if (!isOpen() && fdptr_->isValid())
		return;

	InetAddress peer;
	FD fd = sock_accept(*fdptr_, peer);

	if (cbAccept_)
	{
		cbAccept_(TcpClientPtr(new TcpClient(fd, loop_, peer)));
	}
	else closefd(fd);
	
	fdptr_->pollRead();
}
