#include "serial.h"

#ifdef WIN32
static const UInt32 gBaudrate[] = {1000000, 921600, 576000, 500000, 460800, 230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 600, 300};
static const BYTE gByteSize[] = {8, 7, 6, 5};
static const BYTE gParity[] = {NOPARITY, ODDPARITY, EVENPARITY, SPACEPARITY};
static const BYTE gStopBit[] = {ONESTOPBIT, TWOSTOPBITS};
#else
static const speed_t gBaudrate[] = {B1000000, B921600, B576000, B500000, B460800, B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B600, B300};
static const BYTE gByteSize[] = {CS8, CS7, CS6, CS5};
#endif


static const UInt32 QUE_SIZE[] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 960, 480, 240, 120, 60, 30};

////////////////////////////////////////////////////////////////////////

Serial::Serial(int sendBuffInitSize)
	: fd_(INVALID_HANDLE_VALUE)
	, sendBuff_(sendBuffInitSize)
{
#ifdef WIN32
	writePended_ = false;
	ov_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ovRead_.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovWrite_.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	evWrite_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	evClose_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	szInQue_ = szOutQue_ = MAX_QUE_SIZE;
#endif
}

Serial::~Serial(void)
{
	close();
#ifdef WIN32
	CloseHandle(ov_.hEvent);
	CloseHandle(ovRead_.hEvent);
	CloseHandle(ovWrite_.hEvent);
	CloseHandle(evWrite_);
	CloseHandle(evClose_);
#endif
}

bool Serial::open(const char* ttyName, BAUDRATE baudRate, BYTESIZE byteSize, PARITY parity, STOPBIT stopBit)
{
	Lock lock(mutex_);

	if (isOpen())
	{
		LOGW("Serial has been opened!!");
		return false;
	}

#ifdef WIN32
    fd_ = CreateFileA(ttyName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

    if (fd_ == INVALID_HANDLE_VALUE)
		return false;

	COMMTIMEOUTS tmout;
	tmout.ReadIntervalTimeout = 10;
	tmout.ReadTotalTimeoutMultiplier = 10;
	tmout.ReadTotalTimeoutConstant = 10;
	tmout.WriteTotalTimeoutMultiplier = 10;
	tmout.WriteTotalTimeoutConstant = 10;
	if (!SetCommTimeouts(fd_, &tmout))
		LOGW("SetCommTimeouts error !");

	if (!SetCommMask(fd_, EV_RXCHAR)) //|EV_TXEMPTY
		LOGW("SetCommMask error !");

    bakOption_.DCBlength = sizeof(DCB);
    GetCommState(fd_, &bakOption_);
    DCB dcb;
    dcb = bakOption_;
    dcb.BaudRate = gBaudrate[baudRate];
    dcb.ByteSize = gByteSize[byteSize];
    dcb.Parity = gParity[parity];
    dcb.StopBits = gStopBit[stopBit];
	dcb.fRtsControl = RTS_CONTROL_ENABLE;		// set RTS bit high!
    dcb.fOutxCtsFlow = false;
    dcb.fOutxDsrFlow = false;
    dcb.fOutX = false;
    dcb.fInX = false;
	if (!SetCommState(fd_, &dcb))
		LOGW("SetCommState error !");

	szOutQue_ = (QUE_SIZE[baudRate]>MAX_QUE_SIZE)?MAX_QUE_SIZE:QUE_SIZE[baudRate];
	SetupComm(fd_, szInQue_, szOutQue_);
    PurgeComm(fd_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	
	resetOverlapped(ov_);
	resetOverlapped(ovRead_);
	resetOverlapped(ovWrite_);
	ResetEvent(evWrite_);
	writePended_ = false;
	ResetEvent(evClose_);

#else
	fd_ = ::open(ttyName, O_RDWR | O_NOCTTY);
    if (fd_ == INVALID_HANDLE_VALUE)
		return false;

    int save_file_flags;
    save_file_flags = fcntl(fd_, F_GETFL);
    save_file_flags |= O_NONBLOCK;
    fcntl(fd_, F_SETFL, save_file_flags);//非阻塞

    tcflush(fd_, TCIOFLUSH);
    tcgetattr(fd_, &bakOption_);
    struct termios options;
	memset(&options, 0, sizeof(options));
    options.c_cflag |= CLOCAL | CREAD;
    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //设置波特率
    cfsetispeed(&options, gBaudrate[baudRate]);
    cfsetospeed(&options, gBaudrate[baudRate]);
    //设置数据位数
    options.c_cflag |= gByteSize[byteSize];
    //设置奇偶校验
	switch (parity) 
	{   
	case BP_ODD:     
		options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
		options.c_iflag |= INPCK;             /* Disnable parity checking */ 
		break;
	case BP_EVEN:   
		options.c_cflag |= PARENB;     /* Enable parity */    
		options.c_cflag &= ~PARODD;   /* 转换为偶效验*/     
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	default:	/* BP_NON, BP_SPACE, ..*/
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
	}
    //设置停止位
    switch (stopBit)
	{   
	case BS_TWO:    
		options.c_cflag |= CSTOPB;  
		break;
	default:    /* BS_ONE, ..*/
		options.c_cflag &= ~CSTOPB;
	} 
    //
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag  &= ~OPOST;
    // 设置超时
    options.c_cc[VTIME] = 10;        // 设置超时1秒
    options.c_cc[VMIN] = 0;
    tcsetattr(fd_, TCSANOW, &options);
    tcflush(fd_, TCIOFLUSH);
	
	if (pipe(evClose_) < 0)
	{
		LOGE("%s pipe evClose_ error.", __FILE__);
		return false;
	}

	if (pipe(evWrite_) < 0)
	{
		::close(evClose_[0]);
		::close(evClose_[1]);
		LOGE("%s pipe evWrite_ error.", __FILE__);
		return false;
	}
#endif

	sendBuff_.erase();
	if (!thread_.start(boost::bind(&Serial::loop, this)))
	{
		LOGE("Serial thread start error !!");
		close();
		return false;
	}
	
	return true;
}

void Serial::loop()
{

#ifdef WIN32
	static const int WAIT_EVENT = 4;
	HANDLE hEvents[WAIT_EVENT] = {evClose_, ov_.hEvent, evWrite_, ovWrite_.hEvent};
	DWORD evMask;
	DWORD err;
	COMSTAT comstat;
	DWORD CommEvent = 0;
#else
	static const int WAIT_EVENT = 3;
	struct pollfd hEvents[WAIT_EVENT];
	memset (hEvents, 0, sizeof(hEvents));
	hEvents[0].fd = fd_;
	hEvents[1].fd = evClose_[0];
	hEvents[2].fd = evWrite_[0];
#endif

	bool run = true;
	while (run && isOpen())
	{
#ifdef WIN32
		BOOL res = WaitCommEvent(fd_, &evMask, &ov_);
		if (!res)  
		{
			err = GetLastError();
			if (ERROR_IO_PENDING != err && 87 != err)
				LOGW("WaitCommEvent err=%X", err);
			//continue;
		}

		evMask = WaitForMultipleObjects(WAIT_EVENT, hEvents, FALSE, INFINITE);
		if (WAIT_FAILED == evMask || WAIT_TIMEOUT == evMask)
		{
			LOGW("WaitForMultipleObjects err=%X", GetLastError());
			run = false;
			handleFdClose();
			continue;
		}

		evMask = evMask-WAIT_OBJECT_0;
		switch (evMask)
		{
		case 0:			//evClose_
			run = false;
			handleFdClose();
			break;
		case 1:			//ov_.hEvent
			CommEvent = 0;
			GetCommMask(fd_, &CommEvent);
			ClearCommError(fd_, &err, &comstat);
			if ((CommEvent & EV_CTS) && cbEvent_)
				cbEvent_(BE_CTS, comstat.fCtsHold);
			if ((CommEvent & EV_DSR) && cbEvent_)
				cbEvent_(BE_DSR, comstat.fDsrHold);
			if (CommEvent & EV_RXCHAR)
			{
				handleFdRead(comstat.cbInQue);
			}
			break;
		case 2:			//evWrite_
			ClearCommError(fd_, &err, &comstat);
			handleFdWrite(comstat.cbOutQue);
			break;
		case 3:			//ovWrite_.hEvent
			writePended_ = false;
			handleAfterWrite();
			break;
		}
#else
		hEvents[0].revents = hEvents[1].revents = hEvents[2].revents = 0;
		hEvents[0].events = hEvents[1].events = hEvents[2].events = POLLIN | POLLERR;
		
		if (sendBuff_.readableBytes() > 0)
			hEvents[0].events |= POLLOUT;

		int retval = poll (hEvents, WAIT_EVENT, 500);
		if (retval < 0)
		{
			LOGW("poll err=%X", error_n());
			run = false;
			handleFdClose();
			continue;
		}
		else if (retval == 0)
			continue;		//超时

		if (hEvents[0].revents & POLLIN)
		{
//			LOGD("poll 0");
			handleFdRead();
		}

		if (hEvents[1].revents & POLLIN)		//evClose
		{
			LOGD("poll 1");
			run = false;
			UInt64 val = 1;
			read(hEvents[1].fd, &val, sizeof(val));
			handleFdClose();
		}

		if ((hEvents[2].revents & POLLIN))		//evWrite
		{
//			LOGD("poll 2");
			UInt64 val = 1;
			read(hEvents[2].fd, &val, sizeof(val));
			handleFdWrite();
		}

		if ((hEvents[0].revents & POLLOUT))		//evWrite
		{
//			LOGD("POLLOUT");
			handleFdWrite();
		}

#endif
	}
}

void Serial::handleFdRead(int cbInQue)
{
	int len = 0;
	char buf[MAX_QUE_SIZE];

#ifdef WIN32
	if (cbInQue <= 0)
	{
//		LOGW("EV_RXCHAR but cbInQue=0 !!");
		return;
	}
	DWORD err;
	resetOverlapped(ovRead_);
	BOOL res = ReadFile(fd_, buf, szInQue_, (LPDWORD)&len, &ovRead_);
	if (!res)  
	{ 
		DWORD err = GetLastError();
		if (ERROR_IO_PENDING == err)
		{
			res = GetOverlappedResult(fd_, &ovRead_, (LPDWORD)&len, TRUE);		//阻塞至读完
			if (!res)
			{
				LOGW("handleFdRead GetOverlappedResult error !!");
				return;
			}
			
		}
		else
		{
			LOGW("ReadFile error err=%X!!", err);
			return;
		}
	}
#else
    len = read(fd_, buf, MAX_QUE_SIZE);
	if (len <= 0)
	{
		LOGW("ReadFile error err=%d!!", error_n());
		return;
	}
#endif

	if (cbRead_)
		cbRead_(buf, len);					
}

void Serial::handleFdWrite(int cbOutQue)
{
	int len = 0;
	int trySend = 1024;	//szOutQue_-cbOutQue;
	if (trySend > sendBuff_.readableBytes())
		trySend = sendBuff_.readableBytes();

	assert(trySend >= 0);

	
#ifdef WIN32
	{
		Lock lock(mutexSend_);
		ResetEvent(evWrite_);
		if (trySend > 0 && !writePended_)
		{
			resetOverlapped(ovWrite_);
			BOOL res = WriteFile(fd_, sendBuff_.beginRead(), trySend, (LPDWORD)&len, &ovWrite_);
			if (!res)  
			{ 
				DWORD err = GetLastError();
				if (ERROR_IO_PENDING == err)
					writePended_ = true;
				else LOGW("WriteFile error err=%X!!", err);
				return;
			}
			else handleAfterWrite();
		}
	}
#else 
	{
		Lock lock(mutexSend_);
		len = write(fd_, sendBuff_.beginRead(), trySend);
		int err = error_n();
		if (len < 0 && (EAGAIN != err || EWOULDBLOCK != err))
		{
			LOGW("write error err=%d!!", error_n());
			return;
		}
		if (len > 0)
			sendBuff_.eraseFront(len);
	}
	if (cbSend_ && len > 0)
		cbSend_(len);	
#endif
	

}

#ifdef WIN32

void Serial::resetOverlapped(OVERLAPPED& ov)
{
	ResetEvent(ov.hEvent);
	ov.Internal = ov.InternalHigh = ov.Offset = ov.OffsetHigh = 0;
	ov.Pointer = NULL;
}

void Serial::handleAfterWrite()
{
	DWORD len;
	writePended_ = false;
	BOOL res = GetOverlappedResult(fd_, &ovWrite_, (LPDWORD)&len, FALSE);		//
	if (!res)
	{
		LOGW("handleFdWrite GetOverlappedResult error !!");
		return;
	}

	if (cbSend_)
		cbSend_(len);	

	Lock lock(mutexSend_);
	sendBuff_.eraseFront(len);
	if (sendBuff_.readableBytes() > 0)
		SetEvent(evWrite_);
}

#endif

void Serial::handleFdClose()
{
#ifdef WIN32
    PurgeComm(fd_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	SetCommState((HANDLE)fd_, &bakOption_);
	CloseHandle(fd_);
#else 
	tcsetattr(fd_, TCSANOW, &bakOption_);
	::close(fd_);
	::close(evClose_[0]);
	::close(evClose_[1]);
	::close(evWrite_[0]);
	::close(evWrite_[1]);
#endif
	fd_ = INVALID_HANDLE_VALUE;
}

void Serial::signalClose()
{
#ifdef WIN32
	SetEvent(evClose_);
#else 
    UInt64 val = 1;
	write(evClose_[1], &val, sizeof(val));
#endif
}

void Serial::close()
{
	{
		Lock lock(mutex_);
		if (!isOpen())
			return;
	}

	signalClose();
	thread_.stop();
	sendBuff_.erase();
}

int Serial::sendData(const char* data, int len)
{
	{
		Lock lock(mutex_);
		if (!isOpen() || len <= 0 || data == NULL)
			return -1;
	}

	Lock lock(mutexSend_);

	if (MAX_SENDBUFF_SIZE <= sendBuff_.readableBytes())
		return 0;

	sendBuff_.pushBack(data, len);
#ifdef WIN32
	SetEvent(evWrite_);
#else 
    UInt64 val = 1;
	write(evWrite_[1], &val, sizeof(val));
#endif

	return len;
}
