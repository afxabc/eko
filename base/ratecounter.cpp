#include "ratecounter.h"

RateCounter::RateCounter(UInt32 span)
	: totalByte_(0)
	, span_(span)
{
}

void RateCounter::count(UInt32 len)
{
	Lock lock(mutex_);

	last_ = Timestamp::NOW();
	queueTime_.putBack(last_);
	if (queueTime_.size() == 1)
		first_ = last_;

	queueByte_.putBack(len);
	totalByte_ += len;

	assert(queueTime_.size() == queueByte_.size());

	if (queueByte_.size() > span_)
	{
		queueTime_.getFront(first_);
		queueByte_.getFront(len);
		totalByte_ -= len;
	}
}

void RateCounter::reset()
{ 
	Lock lock(mutex_);
	queueTime_.clear();
	queueByte_.clear();
	totalByte_ = 0;
}

UInt32 RateCounter::bytesPerSecond()
{
	last_ = Timestamp::NOW();
	if (first_ >= last_)
		return 0;

	return (totalByte_*1000)/(last_-first_);
}