#ifndef NET_POLLER_SIG_H_
#define NET_POLLER_SIG_H_

#include "pollerfd.h"
#include "base/timestamp.h"
#include "base/mutex.h"

class PollerLoop;

class PollerSig
{
public:
	PollerSig(PollerLoop* loop);
	~PollerSig(void);

	bool open();
	void close();
	void signal();

	bool isOpen()
	{
		return fdptr_->isValid();
	}

	FD fd()
	{
		return fdptr_->fd();
	}

	int index()
	{
		return fdptr_->index();
	}

private:
	void handleFdRead();

private:
	Mutex mutex_;
	PollerLoop* loop_;
	PollerFdPtr fdptr_;

#ifdef SIG_PIPE
	int fdPipe_[2];
#endif
};

typedef boost::shared_ptr<PollerSig> SigPtr;

#endif
