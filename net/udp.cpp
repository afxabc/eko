#include "udp.h"
#include "pollerloop.h"

Udp::Udp(PollerLoop* loop, UInt32 bufSize, UInt32 sendPend)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
	, charBuf_(new char[bufSize])
	, bufSize_(bufSize)
	, isOpen_(false)
{
	sock_init();
	sendPend_ = (sendPend>0)?sendPend:1;
	fdptr_->setReadCallback(boost::bind(&Udp::handleFdRead, this));
	fdptr_->setWriteCallback(boost::bind(&Udp::handleFdWrite, this));
}

Udp::~Udp(void)
{
	close();
}

bool Udp::open(InetAddress local)
{
	if (!loop_->isInLoopThread())
	{
		if (isOpen())
			return false;
		isOpen_ = true;
		loop_->runInLoop(boost::bind(&Udp::open, this, local));
		return true;
	}

	isOpen_ = true;
	if (fdptr_->isValid())
		return false;

	local_ = local;

	FD fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ( fd == INVALID_FD )
	{
		LOGE("%s socket error.", __FILE__);
		isOpen_ = false;
		return false;
	}

    fd_nonblock(fd, 1);
	if (!local_.isNull())
	{
		if (sock_bind(fd, local_) != 0)
		{
			LOGE("%s socket bind.", __FILE__);
			closefd(fd);
			isOpen_ = false;
			return false;
		}
	}

	*fdptr_ = fd;
	fdptr_->pollRead();

	return true;
}

void Udp::close()
{
	if (!isOpen())
		return;
	isOpen_ = false;

	if (!loop_->isInLoopThread())
	{
		loop_->runInLoop(boost::bind(&Udp::closeInLoop, this));
		fdptr_->waitForClose();
	}
	else closeInLoop();
}

void Udp::closeInLoop()
{
	assert(loop_->isInLoopThread());

	isOpen_ = false;

	{
		Lock lock(mutexSend_);
		sendQueue_.clear();
	}

	fdptr_->close();
}

int Udp::sendData(InetAddress peer, const char* data, int len)
{
	if (!isOpen())
		return -1;

	int ret = len;
	{
		Lock lock(mutexSend_);
		peer_ = peer;
		if (sendQueue_.size() < sendPend_)
		{
			Buffer buff(data, len);
			sendQueue_.putBack(buff);
		}
		else ret = 0;
	}

	if (ret > 0)
		fdptr_->pollWrite(true, true);	//for win32, force to write

	return ret;
}

void Udp::handleFdRead()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

	UInt32 len = sock_recvlength(*fdptr_);
	while (len > 0)
	{
		InetAddress addr;
		int sz = sock_recvfrom(*fdptr_, addr, charBuf_.get(), bufSize_);
		if (sz <= 0)
		{
			LOGE("sock_recvfrom error %d.", error_n());
			break;
		}

		if (cbRead_)
			cbRead_(addr, charBuf_.get(), sz);

		len = sock_recvlength(*fdptr_);
	}
}

void Udp::handleFdWrite()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

	//try send
	int slen = 0;
	{
		Buffer buff;
		Lock lock(mutexSend_);
		while (sendQueue_.size() > 0)
		{
			int len = 0;
			if (sendQueue_.getFront(buff))
			{
                len = sock_sendto(*fdptr_, peer_, buff.beginRead(), buff.readableBytes());

                if (len <= 0)
				{
					sendQueue_.putFront(buff);
					break;
//					LOGE("sock_sendto return %d, error %d.", len, error_n());
				}
                else slen += len;
			}
		}

		if (sendQueue_.size() == 0)
			fdptr_->pollWrite(false);
		else fdptr_->pollWrite(true);
	}

//	if (cbSend_)
//		cbSend_(slen);
}
