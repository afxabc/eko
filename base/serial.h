#ifndef NET_POLLER_SERIAL_H_
#define NET_POLLER_SERIAL_H_

#include "buffer.h"
#include "signal.h"
#include "timestamp.h"
#include "functorloop.h"

#ifndef WIN32
#include	<termios.h>    /*PPSIX 终端控制定义*/
#include	<poll.h>
#include	<unistd.h>
#include	<sys/ioctl.h>
#endif


typedef boost::function<void(char*, int)> SerialReadCallback;
typedef boost::function<void(int)> SerialSendCallback;
typedef boost::function<void(UInt16, UInt32)> SerialEventCallback;

class Serial
{
public:
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
//win32 8,7,6,5
//linux CS8,CS7,CS6,CS5
enum BYTESIZE
{
	BB_8,
	BB_7,
	BB_6,
	BB_5
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

//串口信号
//CTS, BREAK, ERR, RING
//linux 
enum BEVENTS
{
	BE_CTS,
	BE_DSR,
};

#ifndef WIN32
	typedef int HANDLE;
#define INVALID_HANDLE_VALUE -1
#endif

public:
	Serial(int sendBuffInitSize = 4096);
	~Serial(void);

public:
	bool open(const char* ttyName, BAUDRATE baudRate = BR_115200, BYTESIZE byteSize = BB_8, PARITY parity = BP_NON, STOPBIT stopBit = BS_ONE);
	void close();
	int sendData(const char* data, int len);

	bool isOpen()
	{
		return (fd_!=INVALID_HANDLE_VALUE);
	}

	void setReadCallback(const SerialReadCallback& cb)
	{
		cbRead_ = cb;
	}

	void setSendCallback(const SerialSendCallback& cb)
	{
		cbSend_ = cb;
	}

	void setEventCallback(const SerialEventCallback& cb)
	{
		cbEvent_ = cb;
	}

	HANDLE fd() const
	{ return fd_; }

private:
	void loop();
	void signalClose();

	void handleFdRead(int cbInQue = 0);
	void handleFdWrite(int cbOutQue = 0);
	void handleFdClose();

#ifdef WIN32
	void handleAfterWrite();
	void resetOverlapped(OVERLAPPED& ov);
#endif

private:
	HANDLE fd_;
	Thread thread_;
	Mutex mutex_;

	SerialReadCallback cbRead_;
	SerialSendCallback cbSend_;
	SerialEventCallback cbEvent_;

	static const int MAX_SENDBUFF_SIZE = 4*1024;
	Mutex mutexSend_;
	Buffer sendBuff_;

	static const int MAX_QUE_SIZE = 1024;
#ifdef WIN32
	OVERLAPPED ov_;
	OVERLAPPED ovRead_;
	OVERLAPPED ovWrite_;
	HANDLE evWrite_;
	HANDLE evClose_;
	DCB bakOption_;
	int szInQue_;
	int szOutQue_;
	char outQueBuff_[MAX_QUE_SIZE];
	bool writePended_;
#else
	struct termios bakOption_;
	int evClose_[2];
	int evWrite_[2];
#endif

};

typedef boost::shared_ptr<Serial> SerialPtr;

#endif
