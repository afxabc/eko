#include "serial.h"


#ifdef WIN32
static UInt32 gBaudrate[] = {1000000, 921600, 576000, 500000, 460800, 230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 600, 300};
static BYTE gByteSize[] = {8, 7, 6, 5, 4};
static BYTE gParity[] = {NOPARITY, ODDPARITY, EVENPARITY, SPACEPARITY};
static BYTE gStopBit[] = {ONESTOPBIT, TWOSTOPBITS};
#else
static speed_t gBaudrate[] = {B1000000, B921600, B576000, B500000, B460800, B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B600, B300};
static BYTE gByteSize[] = {CS8, CS7, CS6, CS5, CS4};
#endif


////////////////////////////////////////////////////////////////////////

Serial::Serial(int sendBuffInitSize)
	: fd_(INVALID_HANDLE_VALUE)
	, sendBuff_(sendBuffInitSize)
{
#ifdef WIN32
	ov_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ovRW_.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	evWrite_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	evClose_ = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
}

Serial::~Serial(void)
{
	close();
#ifdef WIN32
	CloseHandle(ov_.hEvent);
	CloseHandle(ovRW_.hEvent);
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
	tmout.ReadIntervalTimeout = 1000;
	tmout.ReadTotalTimeoutMultiplier = 1000;
	tmout.ReadTotalTimeoutConstant = 1000;
	tmout.WriteTotalTimeoutMultiplier = 1000;
	tmout.WriteTotalTimeoutConstant = 1000;
	if (!SetCommTimeouts(fd_, &tmout))
		LOGW("SetCommTimeouts error !");

	if (!SetCommMask(fd_, EV_CTS|EV_DSR|EV_RXCHAR)) //|EV_TXEMPTY
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

    PurgeComm(fd_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	
#else
    fd_ = open(ttyName, O_RDWR | O_NOCTTY);
    if (fd_ == INVALID_HANDLE_VALUE)
		return false;

    int save_file_flags;
    save_file_flags = fcntl(mFd, F_GETFL);
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
#endif

	sendBuff_.erase();
	ResetEvent(ov_.hEvent);
	ResetEvent(ovRW_.hEvent);
	ResetEvent(evWrite_);
	ResetEvent(evClose_);
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
	HANDLE hEvents[3] = {evClose_, ov_.hEvent, evWrite_};
	DWORD evMask;
	DWORD err;
	COMSTAT comstat;
	DWORD CommEvent = 0;
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

		evMask = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
		if (WAIT_FAILED == evMask || WAIT_TIMEOUT == evMask)
		{
			LOGW("WaitForMultipleObjects err=%X", GetLastError());
			break;
		}

		evMask = evMask-WAIT_OBJECT_0;
		switch (evMask)
		{
		case 0:			//evClose_
			run = false;
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
				if (comstat.cbInQue > 0)
					handleFdRead();
				else LOGW("EV_RXCHAR but cbInQue=0 !!");
			}
			break;
		case 2:			//evWrite_
			handleFdWrite();
			break;
		}
#else

#endif
	}
}

void Serial::handleFdRead()
{
	int len = 0;
	static const int MAX_BUF = 64;
	char buf[MAX_BUF];

#ifdef WIN32
	DWORD err;
	ResetEvent(ovRW_.hEvent);
	BOOL res = ReadFile(fd_, buf, MAX_BUF, (LPDWORD)&len, &ovRW_);
	if (!res)  
	{ 
		DWORD err = GetLastError();
		if (ERROR_IO_PENDING == err)
		{
			res = GetOverlappedResult(fd_, &ovRW_, (LPDWORD)&len, TRUE);		//阻塞至读完
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
    len = read(fd_, buf, MAX_BUF);
	if (len <= 0)
	{
		LOGW("ReadFile error err=%d!!", error_n());
		return;
	}
#endif

	if (cbRead_)
		cbRead_(buf, len);					
}

void Serial::handleFdWrite()
{
	int len = 0;
	static const int MAX_SEND = 64;
	int trySend = MAX_SEND;
	if (trySend > sendBuff_.readableBytes())
		trySend = sendBuff_.readableBytes();

	{
	Lock lock(mutexSend_);
#ifdef WIN32
	ResetEvent(ovRW_.hEvent);
	BOOL res = WriteFile(fd_, sendBuff_.beginRead(), trySend, (LPDWORD)&len, &ovRW_);
	if (!res)  
	{ 
		DWORD err = GetLastError();
		if (ERROR_IO_PENDING == err)
		{
			res = GetOverlappedResult(fd_, &ovRW_, (LPDWORD)&len, TRUE);		//阻塞至读完
			if (!res)
			{
				LOGW("GetOverlappedResult error !!");
				return;
			}
		}
		else
		{
			LOGW("WriteFile error err=%X!!", err);
			return;
		}
	}
	sendBuff_.eraseFront(len);
	if (sendBuff_.readableBytes() == 0)
		ResetEvent(evWrite_);
#else 
    len = write(fd_, sendBuff_.beginRead(), trySend);
	if (len <= 0)
	{
		LOGW("WriteFile error err=%d!!", error_n());
		return;
	}
	sendBuff_.eraseFront(len);
#endif
	}

	if (cbSend_)
		cbSend_(len);	
}

void Serial::signalClose()
{
#ifdef WIN32
	SetEvent(evClose_);
#else 
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

#ifdef WIN32
	SetCommState((HANDLE)fd_, &bakOption_);
	CloseHandle(fd_);
#else 
	tcsetattr(fd_, TCSANOW, &bakOption_);
	close(fd_);
#endif

	fd_ = INVALID_HANDLE_VALUE;
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
#endif

	return len;
}
