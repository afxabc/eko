#include "base/common.h"
#include "base/log.h"

extern void test_queue();
extern void test_buffer();
extern void test_bits();
extern void test_functor();
extern void test_functorloop();
extern void test_signal();
extern void test_udp(const char*);
extern void test_tcp(const char*);
extern void test_serial(const char*);

int main(int argc, const char* argv[])
{
    LOGLEVEL(DEBUG);

//  test_queue();
//	test_buffer();
//	test_functor();
//	test_functorloop();
//	test_signal();
//	test_udp(argv[1]);
//	test_tcp(argv[1]);
//	test_bits();
	test_serial(argv[1]);

#ifdef WIN32
    system("pause");
#endif

    return 0;
}
