#include "base/common.h"
#include "base/log.h"

extern void test_queue();
extern void test_buffer();
extern void test_functor();
extern void test_functorloop();

int main()
{
	LOG(INFO);

//	test_queue();
//	test_buffer();
//	test_functor();
	test_functorloop();

#ifdef WIN32
	system("pause");
#endif

	return 0;
}