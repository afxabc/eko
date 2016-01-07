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

Serial::Serial(PollerLoop* loop, int sendBuffInitSize)
	: loop_(loop)
	, fdptr_(new PollerFd(loop))
	, sendBuff_(sendBuffInitSize)
	, isOpen_(false)
{
}

Serial::~Serial(void)
{
	close();
}

bool Serial::open(const char* ttyName, BAUDRATE baudRate, BYTESIZE byteSize, PARITY parity, STOPBIT stopBit)
{
	if (isOpen())
		return false;

	FD fd = INVALID_FD;

	//open fd ...
	
#ifdef WIN32
    HANDLE h = CreateFileA(ttyName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

    if (h == INVALID_HANDLE_VALUE)
		return false;
#else
    fd = open(ttyName, O_RDWR | O_NOCTTY);
    if (h == INVALID_FD)
		return false;

    int save_file_flags;
    save_file_flags = fcntl(mFd, F_GETFL);
    save_file_flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, save_file_flags);//非阻塞
#endif

#ifdef WIN32
    DCB mDCB;
	ZeroMemory(&mDCB, sizeof(DCB));
    mDCB.DCBlength = sizeof(DCB);
    GetCommState(h, &mDCB);
    //设置波特率
    mDCB.BaudRate = gBaudrate[baudRate];
    //设置数据位数
    mDCB.ByteSize = gByteSize[byteSize];
    //设置奇偶校验
    mDCB.Parity = gParity[parity];
    //设置停止位
    mDCB.StopBits = gStopBit[stopBit];
    //设置流控
    mDCB.fOutxCtsFlow = false;
    mDCB.fOutxDsrFlow = false;
    mDCB.fOutX = false;
    mDCB.fInX = false;
	if(!SetCommState(h, &mDCB))
	{
		LOGW("SetCommState error!!!!!!");
	}
    PurgeComm(h, PURGE_TXCLEAR|PURGE_RXCLEAR);
	
	fd = (FD)h;
#else
    tcflush(fd, TCIOFLUSH);
    struct termios options;
    tcgetattr(fd, &options);
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
    tcsetattr(fd, TCSANOW, &options);
    tcflush(fd, TCIOFLUSH);
#endif

	isOpen_ = true;

	if (!loop_->isInLoopThread())
		loop_->runInLoop(boost::bind(&Serial::registerInLoop, this, fd));
	else registerInLoop(fd);

	return true;
}

void Serial::registerInLoop(FD fd)
{
	assert(loop_->isInLoopThread());
	assert(!fdptr_->isValid());

	*fdptr_ = fd;
	fdptr_->pollRead();
}

void Serial::close()
{
	if (!isOpen())
		return;
	isOpen_ = false;

	if (!loop_->isInLoopThread())
	{
		loop_->runInLoop(boost::bind(&TcpClient::closeInLoop, this));
		fdptr_->waitForClose();
	}
	else closeInLoop();
}

void Serial::closeInLoop()
{
	assert(loop_->isInLoopThread());

	isOpen_ = false;

	fdptr_->close();

	Lock lock(mutexSend_);
	sendBuff_.erase();
}

int Serial::sendData(const char* data, int len)
{
	if (!isOpen() || len <= 0 || data == NULL)
		return -1;

	int ret = len;
	{
		Lock lock(mutexSend_);

		if (sendBuff_.readableBytes() < MAX_SENDBUFF_SIZE)
			sendBuff_.pushBack(data, len, true);
		else ret = 0;
	}

	fdptr_->pollWrite(true, true);	//for win32, force to write

	return ret;
}

void Serial::handleFdRead()
{
	if (!isOpen())
		return;

	assert(loop_->isInLoopThread());

	char buf[4096];
	int len = recv(*fdptr_, buf, 4096, 0);

	if (len <= 0)
	{
		handleConnectResult(false);
		return;
	}

	if (cbRead_)
		cbRead_(buf, len);
}

void TcpClient::sendBuffer()
{
	assert(loop_->isInLoopThread());
	//try send
	int slen = 0;
	{
		Lock lock(mutexSend_);

		int buffSize = sendBuff_.readableBytes();

		if (buffSize > 0)
		{
			slen = sock_send(*fdptr_, sendBuff_);

			if (slen < 0 )
			{
				handleConnectResult(false);
				return;
			}

			if (slen > 0)
				sendBuff_.eraseFront(slen);

			if (sendBuff_.readableBytes() == 0)
				fdptr_->pollWrite(false);
			else fdptr_->pollWrite();

		}
	}

	if (slen > 0 && cbSend_)
		cbSend_(slen);
}

void TcpClient::handleFdWrite()
{
	if (!isOpen())
		return;

	assert(loop_->isInLoopThread());

	if (conn_ == CONNECTING)
		handleConnectResult(true);
	else if (conn_ == CONNECTED)
		sendBuffer();

}
