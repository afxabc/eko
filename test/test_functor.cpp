#include "base/functorqueue.h"
#include "base/log.h"
#include "base/thread.h"

using namespace std;

class XData
{
public:
	XData(int val = 0) : val_(val) {}

	XData(const XData& data)
	{ copy(data); }

	XData& operator=(const XData& data)
	{ 
		copy(data);
		return *this;
	}

	int val() const
	{ return val_; }

private:
	void copy(const XData& data)
	{
//		LOGI("XData copy .");
		val_ = data.val_;
	}
	int val_;
};

static int TEST = 0;
static void hello(const XData& data)
{
	LOGD("hello : %d", data.val());
	TEST++;
}

void test_functor()
{
	LOGI("\n======= test_functor =======");

	FunctorQueue functors;
	int ret;

	LOGI("post 999 ...");
	functors.post(boost::bind(hello, P(XData(999))));
	TEST = 0;
	LOGI("run ...");
	ret = functors.run();
	assert(ret == 0 && TEST == 1);

	TEST = 0;
	LOGI("run ...");
	ret = functors.run();
	assert(ret == 0 && TEST == 0);

	UInt32 tm1 = functors.post(boost::bind(hello, P(XData(100))), 2000);
	LOGI("post 100 (seq=%d), 2s .", tm1);
	UInt32 tm2 = functors.post(boost::bind(hello, P(XData(200))), 3000);
	LOGI("post 200 (seq=%d), 3s .", tm2);
	UInt32 tm3 = functors.post(boost::bind(hello, P(XData(300))), 1000);
	LOGI("post 300 (seq=%d), 1s .\n", tm3);

	LOGI("run now ...");
	TEST = 0;
	ret = functors.run();
	if (ret > 0)
		LOGD("Next run after %d ms.\n", ret);
	functors.dump();
	assert(ret > 0 && TEST == 0 && functors.size() == 3);

	LOGI("cancel (seq=%d) ...", tm2);
	functors.cancel(tm2);
	functors.dump();
	assert(functors.size() == 2);

	LOGI("sleep %ds ...", ret);
	Thread::sleep(ret+10);
	LOGI("run now ...");
	TEST = 0;
	ret = functors.run();
	if (ret > 0)
		LOGD("Next run after %d ms.\n", ret);
	functors.dump();
	assert(ret > 0 && functors.size() == 1);

	LOGI("sleep %ds ...", ret);
	Thread::sleep(ret+10);
	LOGI("run now ...");
	TEST = 0;
	ret = functors.run();
	if (ret > 0)
		LOGD("Next run after %d ms.\n", ret);
	assert(ret == 0 && functors.size() == 0);

	LOGI("post 400, 3s ...");
	functors.post(boost::bind(hello, P(XData(400))), 3000);
	LOGI("post 500, 3s ...");
	functors.post(boost::bind(hello, P(XData(500))), 3000);
	LOGI("post 600, 3s ...\n");
	functors.post(boost::bind(hello, P(XData(600))), 3000);

	LOGI("cancel (seq=%d) ...", tm2);
	functors.cancel(tm2);
	functors.dump();
	assert(functors.size() == 3);

	LOGI("run now ...");
	TEST = 0;
	ret = functors.run();
	if (ret > 0)
		LOGD("Next run after %d ms.\n", ret);
	functors.dump();
	assert(functors.size() == 3 && ret > 0 && TEST == 0);

	LOGI("sleep %ds ...", ret);
	Thread::sleep(ret+10);
	LOGI("run now ...");
	TEST = 0;
	ret = functors.run();
	assert(ret == 0 && TEST == 3);

}