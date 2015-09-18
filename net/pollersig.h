#ifndef NET_POLLER_SIG_H_
#define NET_POLLER_SIG_H_

#include "pollerfd.h"
#include "base/timestamp.h"
#include "base/mutex.h"

class PollerLoop;

#ifdef WIN32
#define SIG_UDP
#endif

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

private:
	void handleFdRead(Timestamp receiveTime);

private:
	Mutex mutex_;
	PollerLoop* loop_;
	PollerFdPtr fdptr_;

#ifdef SIG_UDP
	InetAddress addr_;
#endif

#ifdef SIG_PIPE
	int fdPipe_[2];
#endif
};

typedef boost::shared_ptr<PollerSig> SigPtr;

#endif
