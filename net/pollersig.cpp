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
	sock_init();
	fdptr_->setReadCallback(boost::bind(&PollerSig::handleFdRead, this));
}

PollerSig::~PollerSig(void)
{
}

void PollerSig::handleFdRead()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
		return;

#ifndef WIN32
    UInt64 val = 1;
	read(*fdptr_, &val, sizeof(val));
#endif
}

bool PollerSig::open()
{
	assert(loop_->isInLoopThread());

	if (!isOpen())
	{
#ifdef WIN32
		FD fd = SIG_FD;
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

	fdptr_->pollRead();

	return true;
}

void PollerSig::close()
{
	assert(loop_->isInLoopThread());

	if (isOpen())
	{
		loop_->removePoll(fdptr_);
#ifndef WIN32 
		closefd(*fdptr_);
#endif
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
#ifdef WIN32
//	sock_sendto(*fdptr_, addr_, (char*)&val, sizeof(val));
#elif defined(SIG_PIPE)
	write(fdPipe_[1], &val, sizeof(val));
#else
	write(*fdptr_, &val, sizeof(val));
#endif
}
