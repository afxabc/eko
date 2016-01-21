#include "base/buffer.h"
#include "base/log.h"

#include <string>

using namespace std;

static BYTE bitChar[256];
static void initBitChar()
{
	static Mutex mutex;
	static bool init = false;
	{
		Lock lock(mutex);
		if (init)
			return;
		init = true;
	}

	for (int i=0 ;i<256; i++)
	{
		BYTE b = i;
		bitChar[i] = 0;
		for (int j=0; j<8; j++)
		{
			bitChar[i] += b&1;
			b >>= 1;
		}
	}
}

static int leftMoveCompare(const void* p0, const void* pp1, BYTE count, BYTE move)
{
	static const BYTE out[8] = {0, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
	static const BYTE mark[8] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};
	int err = 0;
	const BYTE* c0 = (const BYTE*)p0;
	const BYTE* p1 = (const BYTE*)pp1;
	BYTE c1[512];
	move = move%8;

	BYTE o = 0;
	for (int i=0; i<count+1; i++)
	{
		BYTE oo = (p1[i]&out[move])>>(8-move);
		c1[i] = p1[i] << move;
		if (i > 1)
			c1[i-1] = (c1[i-1]&mark[move]|o);
		o = oo;
	}

	for (int i=0; i<count; i++)
	{
		BYTE c = (c0[i]^c1[i]);
		err += bitChar[c];
	}

	if (err > 0)
	{
		err += 0;
	}

	return err;
}

static int compareBuffer(const Buffer& sendBuff, const Buffer& recvBuff, int nn)
{

	static const BYTE SYNC = 32;
	static const BYTE SPAN = 2;

	int total = sendBuff.readableBytes()-1;
	if (total == 0)
		return 0;

	int rtotal = recvBuff.readableBytes()-1;

	const char* pSrc = sendBuff.beginRead();
	const char* pDst = recvBuff.beginRead();

	int szSrc;
	int szDst;

	int err = 0;
	char bmove = -1; 
	bool end = false;
	bool sync = false;

	while (!end)
	{
		szSrc = total-(pSrc-sendBuff.beginRead());
		szDst = rtotal-(pDst-recvBuff.beginRead());

		int sync_span = SYNC;
		if (sync_span > szDst)
			sync_span = szDst;
		if (sync_span > szSrc)
			sync_span = szSrc;
		if (sync_span <= 0)
			break;

		while (bmove < 0)
		{
			for (BYTE i=0; i<8; i++)
			{
				int e = leftMoveCompare(pSrc, pDst, sync_span, i);
				if (e == 0)
				{
					bmove = i;
					sync = true;
					break;
				}
			}

			if (bmove >= 0)
				break;

			pDst++;
			szDst--;
			if (sync)
				err += 8;

			if (szDst <= 0)
			{
				end = true;
				break;
			}
		}

		if (end)
			break;

		int szMin = (szSrc>szDst)?szDst:szSrc;

		while (szMin >= 1)
		{
			int e = 0;
			int k = pDst-recvBuff.beginRead();
			if (nn <=  k)
				e = 0;

			int span = (szMin>SPAN)?SPAN:szMin;
			e = leftMoveCompare(pSrc, pDst, span, bmove);
			if (e >= span)
			{
				bmove = -1;
				break;
			}

			if (e > 0)
				err += e;

			pDst += span;
			pSrc += span;
			szMin -= span;
			if (szMin <= 0)
				end = true;
		}
	}
	
	szSrc = total-(pSrc-sendBuff.beginRead());
	szDst = rtotal-(pDst-recvBuff.beginRead());

	if (szSrc > szDst)
	{
		err += (szSrc-szDst)*8;
	}

	return err;

}

void print_2(int val2)
{
	for (int i = 7; i >= 0; i--)
	{
		if(val2 & (1 << i))
			printf("1");
		else
			printf("0");
	}
}

#ifdef WIN32
#include <winbase.h>
#endif

void test_bits()
{
	initBitChar();
	
	for (int i=0 ;i<256; i++)
	{
		if (i%8 == 0)
			printf("\n");
		print_2(i);
		printf("=%02X[%d] ", i, bitChar[i]);
	}
	printf("\n\n");


	Buffer sBuff;
	Buffer rBuff;

	int err;
	static const BYTE mark[8] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80}; 
	char b1[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
	char b2[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
	b1[1] ^= mark[0];	//mark[k];
	sBuff.pushBack(b1, sizeof(b1), true);
	rBuff.pushBack(b2, sizeof(b2), true);
	err = compareBuffer(sBuff, rBuff, 1);


	static const int BUF_SIZE = 10000;
	char buf[BUF_SIZE];
	for (int i=0; i<BUF_SIZE; i++)
		buf[i] = i;

	sBuff.erase();
	rBuff.erase();
	sBuff.pushBack(buf, BUF_SIZE, true);
	rBuff.pushBack(buf, BUF_SIZE, true);
	
	static const int TMP_SIZE = 73;
	char tmp[TMP_SIZE];
	memset(tmp, 0x80, TMP_SIZE);
	rBuff.pushFront(tmp, TMP_SIZE, true);

	err = compareBuffer(sBuff, rBuff, 1);
	assert(err==0);

#ifdef WIN32
	srand(GetTickCount());
#else
	srand((unsigned)time(NULL));
#endif

	char* pb = rBuff.beginRead();
	char* ps = sBuff.beginRead();

	static const int BUF_SIZE_2 = BUF_SIZE/2;
	int n = rand()%BUF_SIZE_2;
	int k = rand()%8;
	char b = pb[n];
	pb[n] ^= mark[0];	//mark[k];
	char c = ps[n-TMP_SIZE];
	err = compareBuffer(sBuff, rBuff, n);
	assert(err==1);

	static const int N = 9;
	for (int i=0; i<N; ++i)
	{
	int m = rand()%BUF_SIZE_2+BUF_SIZE_2;
	k = rand()%8;
	pb[m] ^= mark[k];	//mark[k];
	c = ps[m-TMP_SIZE];
	}
	err = compareBuffer(sBuff, rBuff, 0);
	assert(err==(1+N));

	rBuff.erase();
	rBuff.pushBack(buf, BUF_SIZE/4, true);
	rBuff.pushBack(tmp, TMP_SIZE, true);
	rBuff.pushBack(buf+BUF_SIZE/2, BUF_SIZE/2, true);
	err = compareBuffer(sBuff, rBuff, n);
	assert(err>=TMP_SIZE*8);
}
