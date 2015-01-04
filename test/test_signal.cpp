#include "base/signal.h"
#include "base/log.h"

#include <string>

using namespace std;


static void CHECK(bool condition, const char* sformat, ...)
{
    if (!condition)
    {
        va_list ap;
        va_start(ap, sformat);
        LOGE(sformat, ap);
        va_end(ap);
    }
    assert(condition);
}

void test_signal()
{
    LOGI("\n======= test_signal =======");


    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    static Signal signal_;
    signal_.off();

    bool ret = signal_.wait(1000);
    CHECK(ret, "callback timeout!!");

}
