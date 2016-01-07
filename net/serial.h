#ifndef NET_POLLER_SERIAL_H_
#define NET_POLLER_SERIAL_H_

#include "pollerfd.h"
#include "base/buffer.h"
#include "base/signal.h"
#include "base/timestamp.h"

class PollerLoop;

typedef boost::function<void(char*, int)> SerialReadCallback;
typedef boost::function<void(int)> SerialSendCallback;

class Serial
{
//波特率
enum BAUDRATE
{
	BR_1000000,
	BR_921600,
	BR_576000,
	BR_500000,
	BR_460800,
	BR_230400,
	BR_115200,
	BR_57600,
	BR_38400,
	BR_19200,
	BR_9600,
	BR_4800,
	BR_2400,
	BR_1200,
	BR_600,
	BR_300,
	BR_END,
};

//数据位数
//win32 8,7,6,5,4
//linux CS8,CS7,CS6,CS5,CS4
enum BYTESIZE
{
	BB_8,
	BB_7,
	BB_6,
	BB_5,
	BB_4
};

//奇偶校验
//win32 NOPARITY,ODDPARITY,EVENPARITY,SPACEPARITY
//linux 
enum PARITY
{
	BP_NON,
	BP_ODD,
	BP_EVEN,
	BP_SPACE
};

//停止位
//win32 ONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS
//linux 
enum STOPBIT
{
	BS_ONE,
	BS_TWO
};

public:
	Serial(PollerLoop* loop, int sendBuffInitSize = 4096);
	~Serial(void);

public:
	bool open(const char* ttyName, BAUDRATE baudRate = BR_115200, BYTESIZE byteSize = BB_8, PARITY parity = BP_NON, STOPBIT stopBit = BS_ONE);
	void close();
	int sendData(const char* data, int len);

	bool isOpen()
	{
		return isOpen_;
	}

	void setReadCallback(const SerialReadCallback& cb)
	{
		cbRead_ = cb;
	}

	void setSendCallback(const SerialSendCallback& cb)
	{
		cbSend_ = cb;
	}

	FD fd() const
	{ return fdptr_->fd(); }

private:
	void registerInLoop(Fd fd);
	void closeInLoop();

	void sendBuffer();
	void handleFdRead();
	void handleFdWrite();

private:
	PollerLoop* loop_;
	PollerFdPtr fdptr_;

	Buffer ttyName_;

	SerialReadCallback cbRead_;
	SerialSendCallback cbSend_;

	static const int MAX_SENDBUFF_SIZE = 256*1024;
	Mutex mutexSend_;
	Buffer sendBuff_;

	bool isOpen_;
};

typedef boost::shared_ptr<Serial> SerialPtr;

#endif
