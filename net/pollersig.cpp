#include "pollersig.h"
#include "pollerloop.h"

#ifndef WIN32
#ifndef SIG_PIPE
#include <sys/eventfd.h>
#endif
#endif

PollerSig::PollerSig(PollerLoop* loop)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
{
	fdptr_->setReadCallback(boost::bind(&PollerSig::handleFdRead, this, _1));
}

PollerSig::~PollerSig(void)
{
}

void PollerSig::handleFdRead(Timestamp receiveTime)
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

#ifdef SIG_UDP
	UInt32 len = sock_recvlength(*fdptr_);
	char buf[8];
	while (len > 0)
	{
		InetAddress addr;
		int sz = sock_recvfrom(*fdptr_, addr, buf, 8);
		if (sz <= 0)
		{
			LOGE("sock_recvfrom error %d.", error_n());
			break;
		}
		len = sock_recvlength(*fdptr_);
	}
#else
    UInt64 val = 1;
	read(*fdptr_, &val, sizeof(val));
#endif
}

bool PollerSig::open()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
	{
#ifdef SIG_UDP
		UInt16 port = 20000+rand()%(Thread::self());
		addr_.update("127.0.0.1", port);
		FD fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if ( fd == INVALID_FD )
		{
			LOGE("%s socket error.", __FILE__);
			return false;
		}
		sock_resuseaddr(fd, 1);
		if (sock_bind(fd, addr_) != 0)
		{
			LOGE("%s socket bind.", __FILE__);
			closefd(fd);
			return false;
		}
#elif defined(SIG_PIPE)
		if (pipe(fdPipe_) < 0)
		{
			LOGE("%s pipe error.", __FILE__);
			return false;
		}
		FD fd = fdPipe_[0];
#else
		FD fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (fd < 0)
		{
			fd = INVALID_FD;
			LOGE("%s eventfd error.", __FILE__);
			return false;
		}
#endif

		*fdptr_ = fd;
	}

	fdptr_->enableReading();

	return true;
}

void PollerSig::close()
{
	assert(loop_->isInLoopThread());

	if (isOpen())
	{
		loop_->removePoll(fdptr_);
		closefd(*fdptr_);
		*fdptr_ = INVALID_FD;
	}

#ifdef SIG_PIPE
	closefd(fdPipe_[1]);
#endif
}

void PollerSig::signal()
{
	if (!isOpen())
		return;

    UInt64 val = 1;
#ifdef SIG_UDP
	sock_sendto(*fdptr_, addr_, (char*)&val, sizeof(val));
#elif defined(SIG_PIPE)
	write(fdPipe_[1], &val, sizeof(val));
#else
	write(*fdptr_, &val, sizeof(val));
#endif
}
