#include "net/udp.h"
#include "net/pollerloop.h"
#include "base/log.h"
#include "base/thread.h"
#include "base/signal.h"
#include "base/md5.h"

#include <string>

static const UInt16 svrPort = 17766;
static InetAddress peer;
static Signal sg;
static bool run = false;
static bool pause_ = false;
static int epnum = 0;
static UInt32 recvCount;
static UInt32 lost;
static Timestamp recvTm;

static void sendThread(UdpPtr uptr)
{
    static const int MAX_BUF = 1500;
    char buf[MAX_BUF+128];

    LOGI("send thread enter.");
    uptr->open(peer.getPort());
    sg.on();
    int num = 0;
    epnum = 0;
    lost = 0;
    recvCount = 0;
    recvTm = Timestamp::NOW();
	pause_ = false;
    while (run)
    {
		if (pause_)
		{
			sg.wait(1000);
			continue;
		}

        int len = rand()%MAX_BUF+64;
        *((int*)buf) = num;
        MD5 md5(buf, len);
        memcpy(buf+len, md5.digest(), 16);
        len += 16;

        int slen = 0;
        do
        {
            slen = uptr->sendData(peer, buf, len);
            if (slen == 0)
            {
                //		LOGI("send pending ...");
                sg.wait(10);
            }
        }
        while (slen == 0 && run);

        num++;
    }
    uptr->close();
    LOGI("send thread quit.");
}

static void handleRead(Timestamp tm, InetAddress addr, Buffer buffer)
{
    int len = buffer.readableBytes();
    char* buf = buffer.beginRead();

    recvCount += len;
    if (recvCount > 0x20000000)
    {
        recvCount = 0;
        recvTm = Timestamp::NOW();
    }

    int num = *((int*)buf);
    if (num > epnum)
    {
        lost += (num-epnum);
//		LOGE("recv num expect %d but %d.", epnum, num);
    }
    epnum = num+1;

    MD5 md5(buf, len-16);
    if (memcmp(buf+len-16, md5.digest(), 16) != 0)
    {
        LOGE("recv num %d MD5 error !!!", num);
    }
//	else LOGI("recv from %s : %d", addr.toString().c_str(), len);
}

void test_udp(const char* str)
{
    sock_init();

    if (!str)
    {
        printf("enter peer ip : ");
        char line[64]; // room for 20 chars + '\0'
        fgets(line, sizeof(line)-1, stdin);
        peer = InetAddress(line, svrPort);
    }
    else peer = InetAddress(str, svrPort);

    PollerLoop loop;
    UdpPtr uptr(new Udp(&loop));
    uptr->setReadCallback(boost::bind(&handleRead, _1, _2, _3));

    Thread thread;

    loop.loopInThread();

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
        case 'l':
        case 'L':
        {
            int ms = Timestamp::NOW()-recvTm;
            if (ms > 0)
            {
                float rate = (float)recvCount/ms;
                char c = 'k';
                if (rate > 1024)
                    rate /= 1024, c = 'M';

				if (epnum > 0)
					LOGI("rate=%.2f%c  lost=%.1f%%(%d/%d)", rate, c, (float)lost*100/epnum, lost, epnum);
				else LOGI("rate=%.2f%c", rate, c);

                recvCount = 0;
                recvTm = Timestamp::NOW();
            }
        }
        break;
        case 'c':
        case 'C':
            run = false;
            sg.on();
            thread.stop();
            break;
        case 'p':
        case 'P':
            pause_ = !pause_;
            sg.on();
			if (pause_)
				LOGI("send pause ...");
			else LOGI("send continue ...");
            break;
        case 'q':
        case 'Q':
            test = false;
            run = false;
            sg.on();
            thread.stop();
            break;
        default:
            if (ch > '0' && ch <= '9')
            {
                run = false;
                sg.on();
                thread.stop();

                uptr.reset(new Udp(&loop, Udp::defaultRecvSize, ch-'0'));
                uptr->setReadCallback(boost::bind(&handleRead, _1, _2, _3));

                run = true;
                thread.start(boost::bind(&sendThread, uptr));
            }
        }
    }

    uptr.reset();
    loop.quitLoop();

}