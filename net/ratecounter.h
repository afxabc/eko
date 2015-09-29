#ifndef NET_RATE_COUNT_H_
#define NET_RATE_COUNT_H_

#include "base/timestamp.h"
#include "base/queue.h"

class RateCounter
{
public:
	RateCounter(UInt32 span = SPAN_DEFAULT);

	void count(UInt32 len);
	void reset();
	UInt32 bytesPerSecond();

private:
	static const UInt32 SPAN_DEFAULT = 100;
	UInt32 span_;
	Queue<Timestamp> queueTime_;
	Timestamp first_;
	Timestamp last_;
	Queue<UInt32> queueByte_;
	UInt32 totalByte_;
	Mutex mutex_;
};

#endif
