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


void print_hex_dat(BYTE *dat, const int len)
{
	printf("-----------------------------------------------\n");
	int llen = len/16;
	BYTE* pl = dat;
	for(int i=0; i<llen; i++)
	{
		printf("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n", 
				(int)pl[0], (int)pl[1], (int)pl[2], (int)pl[3], (int)pl[4], (int)pl[5], (int)pl[6], (int)pl[7], 
				(int)pl[8], (int)pl[9], (int)pl[10], (int)pl[11], (int)pl[12], (int)pl[13], (int)pl[14], (int)pl[15]);
		pl += 16;
	}
	llen = len%16;
	for(int i=0; i<llen; i++)
	{
		printf("%.2X ",(int)pl[0]);
		pl++;
	}
	if (llen > 0)
		printf("\n-----------------------------------------------\n");
	else printf("-----------------------------------------------\n");
}

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
