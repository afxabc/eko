#include "net/tcpclient.h"
#include "net/tcpserver.h"
#include "net/pollerloop.h"
#include "base/log.h"
#include "base/thread.h"
#include "base/signal.h"
#include "base/md5.h"
#include <time.h>

#include <string>

static const UInt16 svrPort = 7788;
static InetAddress svrAddr;
static Signal sg;
static bool run = false;
static int epnum = 0;

static void sendThread(TcpClientPtr uptr)
{
    static const int MAX_BUF = 6500;
    char buf[MAX_BUF+128];

    srand((unsigned int)time(NULL));
    LOGI("send thread enter.");
    uptr->open(svrAddr);
    sg.on();
    int num = 0;
    epnum = 0;
	while (run && uptr->isOpen())
    {
        int len = rand()%MAX_BUF+64;
        int sz = len+16;

        int n = 0;
        buf[n++] = 'X';
        buf[n++] = 'Y';
        //*((int*)(buf+n)) = len+16;
        memcpy(buf+n, &sz, 4);
        //LOGI("send packet size=%X [%X] [%X] [%X] [%X] [%X] [%X] ", sz, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        n+=4;
        //*((int*)(buf+n)) = num;
        memcpy(buf+n, &num, 4);
        MD5 md5(buf, len);
        memcpy(buf+len, md5.digest(), 16);
        len += 16;

        int slen = 0;
        do
        {
            slen = uptr->sendData(buf, len);
            if (slen == 0)
            {
                //		LOGI("send pending ...");
                sg.wait(10);
            }
        }
        while (slen == 0 && run);

        num++;
       //if (num % 10 == 0)

        //sg.wait(1000);
    }
    uptr->close();
    LOGI("send thread quit.");
}

static void handleConnect(InetAddress addr, bool con)
{
    if (con)
    {
        LOGI("connect to %s.", addr.toString().c_str());
    }
    else
    {
        LOGI("can not connect to %s.(%d)", addr.toString().c_str(), error_n());
        run = false;
        sg.on();
    }
}

static Buffer recvBuff;
static UInt32 recvCount;
static Timestamp recvTm;
static void handleRead(Timestamp tm, Buffer buffer)
{
    int len = buffer.readableBytes();
    char* buf = buffer.beginRead();

    assert(len > 0);

    recvCount += len;
    if (recvCount > 0x20000000)
    {
        recvCount = 0;
        recvTm = Timestamp::NOW();
    }

    recvBuff.pushBack(buf, len, true);

    buf = recvBuff.beginRead();
    len = recvBuff.readableBytes();

    int span = 0;
    while (len >= 6)
    {
        if (buf[0]!='X' || buf[1]!='Y')
        {
            buf++, span++, len--;
            continue;
        }

        if (span > 0)
            recvBuff.eraseFront(span);

        //LOGI("hit head!! span=%d", span);

        span = 0;
        buf = recvBuff.beginRead();
        len = recvBuff.readableBytes();

        //int sz = *((int*)(buf+2));
        int sz = 0;
        memcpy(&sz, buf+2, 4);
        //LOGI("packet size=%X [%X] [%X] [%X] [%X] [%X] [%X] ", sz, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        if (len < sz)
            break;

        //int num = *((int*)(buf+6));
        int num = 0;
        memcpy(&num, buf+6, 4);
        if (num != epnum)
            LOGE("recv num expect %d but %d.", epnum, num);
        epnum = num+1;
  //      if (num % 10 == 0)
        //LOGI("recv packet %d .", num);

        MD5 md5(buf, sz-16);
        if (memcmp(buf+sz-16, md5.digest(), 16) != 0)
            LOGE("recv num %d MD5 error !!!", num);

        recvBuff.eraseFront(sz);
        buf = recvBuff.beginRead();
        len = recvBuff.readableBytes();

    }

//	else LOGI("recv from %s : %d", addr.toString().c_str(), len);
}

static TcpClientPtr suptr;
static void handleAccept(TcpClientPtr p)
{
    LOGI("accept from %s.", p->peer().toString().c_str());
    suptr = p;
    suptr->setReadCallback(boost::bind(&handleRead, _1, _2));
}

void test_tcp(const char* str)
{
    sock_init();

    if (!str)
    {
        printf("enter server ip : ");
        char line[64]; // room for 20 chars + '\0'
        fgets(line, sizeof(line)-1, stdin);
        svrAddr = InetAddress(line, svrPort);
    }
    else svrAddr = InetAddress(str, svrPort);

    PollerLoop loop;
    loop.loopInThread();

    TcpServerPtr sptr(new TcpServer(&loop));
//    sptr->open(svrAddr);
    sptr->setAcceptCallback(boost::bind(&handleAccept, _1));

    Thread thread;

    TcpClientPtr uptr(new TcpClient(&loop));
    uptr->setConnectCallback(boost::bind(&handleConnect, _1, _2));

    bool test = true;
    while (test)
    {
        char ch = getchar();
        switch (ch)
        {
        case 'o':
        case 'O':
            run = true;
            thread.start(boost::bind(&sendThread, uptr));
            break;
        case 's':
        case 'S':
            recvCount = 0;
            recvBuff.erase();
            recvTm = Timestamp::NOW();
			sptr->open(svrAddr);
            break;
        case 'l':
        case 'L':
        {
            int ms = Timestamp::NOW()-recvTm;
            if (ms > 0)
            {
                float rate = (float)recvCount/ms;
                if (rate > 1024)
                    LOGI("rate=%.2fM total=%X", rate/1024, recvCount);
                else LOGI("rate=%.2fk total=%X", rate, recvCount);
                LOGI("send buffer pending %d", uptr->getSendPending());
                LOGI("recv buffer pending %d", recvBuff.readableBytes());
                recvCount = 0;
                recvTm = Timestamp::NOW();
            }
        }
        break;
        case 'c':
        case 'C':
            if (suptr)
                suptr->close();
            run = false;
			sg.on();
            thread.stop();
			sptr->close();
            break;
        case 'q':
        case 'Q':
            test = false;
            run = false;
            sg.on();
            thread.stop();
            break;
        }
    }

    sptr.reset();
    suptr.reset();
    uptr.reset();

    loop.quitLoop();
}
