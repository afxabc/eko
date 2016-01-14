
#include "base/log.h"
#include "base/thread.h"
#include "base/signal.h"
#include "base/md5.h"
#include "base/serial.h"
#include "base/ratecounter.h"

#include <string>

static Signal sg;
static bool run = false;
static bool pause_ = false;
static int epnum = 0;
static UInt32 lost;
static RateCounter rateCounter(1024);
static int waitTime_ = 0;

static void sendThread(SerialPtr sptr)
{
    static const int MAX_BUF = 65000;
    char buf[MAX_BUF+128];

    LOGI("send thread enter.");
    sg.on();
    int num = 0;
    epnum = 0;
    lost = 0;
	pause_ = false;
    while (run)
    {
		if (pause_)
		{
			sg.wait(1000);
			continue;
		}

		if (waitTime_ < 0)
			sg.wait();

        int len = rand()%MAX_BUF+64;
        *((int*)buf) = num;
        MD5 md5(buf, len);
        memcpy(buf+len, md5.digest(), 16);
        len += 16;

        int slen = 0;
        do
        {
            slen = sptr->sendData(buf, len);
            if (slen == 0)
            {
             //   LOGI("send pending ...");
                sg.wait(100);
            }
			//else LOGI("send =======");
        }
        while (slen == 0 && run);

        num++;
    }

    LOGI("send thread quit.");
}

static void handleRead(char* buf, int len)
{
	rateCounter.count(len);

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

void test_serial(const char* str)
{
	const char* ttyName = str;
    if (!str)
    {
        printf("enter tty name : ");
        char line[64]; // room for 20 chars + '\0'
        fgets(line, sizeof(line)-1, stdin);
		if (strlen(line) >= 4)
			ttyName = line;
#ifdef WIN32
		else ttyName = "COM4";
#else
		else ttyName = "/dev/ttyS1";
#endif
    }

    SerialPtr sptr(new Serial());
	sptr->open(ttyName);
    sptr->setReadCallback(boost::bind(&handleRead, _1, _2));

    Thread thread;

    bool test = true;
    while (test)
    {
        char ch = getchar();
        switch (ch)
        {
        case 'o':
        case 'O':
            run = true;
			waitTime_ = 0;
            thread.start(boost::bind(&sendThread, sptr));
            break;
        case 'l':
        case 'L':
        {
			float rate = (float)rateCounter.bytesPerSecond()/1000;
            char c = 'k';
            if (rate > 1024)
                rate /= 1024, c = 'M';

			if (epnum > 0)
				LOGI("rate=%.2f%c  lost=%.1f%%(%d/%d)", rate, c, (float)lost*100/epnum, lost, epnum);
			else LOGI("rate=%.2f%c", rate, c);

        }
        break;
        case 'c':
        case 'C':
            run = false;
			waitTime_ = 0;
            sg.on();
            thread.stop();
            break;
        case 's':
        case 'S':
			waitTime_ = -1;
            sg.on();
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
        }
    }

    sptr.reset();

}
