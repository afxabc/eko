#ifndef  EKO_TEST_HEAD__
#define EKO_TEST_HEAD__

#include "base/common.h"
#include "base/buffer.h"
#include "base/md5.h"

class FrameBuffer
{
public:
	FrameBuffer(void);
	~FrameBuffer(void);

	void pushBack(const char* buf, int len)
	{ 
		buffer_.pushBack(buf, len, true); 
	}

	int size()
	{ 
		return buffer_.readableBytes();
	}

	/*
	return
	>0	unwrap OK
	==0 can not unwrap
	<0	bufSize too small,
	*/
	int unwrap(char* outBuf, int bufSize);

private:
	Buffer buffer_;
};

#endif
