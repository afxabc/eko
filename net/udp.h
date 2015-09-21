#ifndef NET_POLLER_UDP_H_
#define NET_POLLER_UDP_H_

#include "pollerfd.h"
#include "base/buffer.h"
#include "base/signal.h"
#include "base/timestamp.h"
#include "base/queue.h"

class PollerLoop;

typedef boost::function<void(Timestamp, InetAddress, Buffer)> UdpReadCallback;

class Udp : public boost::noncopyable
{
public:
	static const UInt32 defaultRecvSize = 0x10000;
	Udp(PollerLoop* loop, UInt32 bufSize = defaultRecvSize, UInt32 sendPend = 1);
	~Udp(void);

	bool open(UInt16 port = 0);
	void close();
	int sendData(InetAddress peer, const char* data, int len);

	bool isOpen()
	{
		return isOpen_;
	}

	void setReadCallback(const UdpReadCallback& cb)
	{ 
		cbRead_ = cb; 
	}

private:
	void handleFdRead(Timestamp receiveTime);
	void handleFdWrite();
	void closeInLoop();

private:
	PollerLoop* loop_;
	PollerFdPtr fdptr_;
	InetAddress local_;
	InetAddress peer_;
	UdpReadCallback cbRead_;

	Mutex mutexSend_;
	UInt32 sendPend_;
	Queue<Buffer> sendQueue_;

	boost::scoped_array<char> charBuf_;
	UInt32 bufSize_;

	bool isOpen_;
};

typedef boost::shared_ptr<Udp> UdpPtr;

#endif