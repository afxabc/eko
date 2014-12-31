#ifndef _BASE_QUEUE_H__
#define _BASE_QUEUE_H__ 

#include <deque>
#include "mutex.h"
#include "signal.h"
#include "log.h"

template <typename QDATA>
class Queue
{
public:
	Queue(unsigned int max = 0xffffffff) : sPut_(true) 
	{ 
		setMax(max);
	}

public:
	bool put(QDATA& data, UInt32 tm_wait = 0, bool back = true);
	bool get(QDATA& data, UInt32 tm_wait = 0, bool front = true);
	void clear();
	
	void setMax(unsigned int max) {
		max_ = max;
		if (max_ == 0)
			max_ = 1;
	}

	void signalPut() {
		sPut_.on();
	}

	void signalGet() {
		sGet_.on();
	}

	void signalAll() {
		sGet_.on();
		sPut_.on();
	}

	size_t size() {
		return queue_.size();
	}

protected:
	typedef std::deque<QDATA> QUEUE;
	QUEUE queue_;
	mutable Mutex mutex_;
	Signal sPut_, sGet_;
	unsigned int max_;
};

template <typename QDATA>
bool Queue<QDATA>::put(QDATA& data, UInt32 tm_wait, bool back)
{
	if (!sPut_.wait(tm_wait))
		return false;

	Lock lock(mutex_);
	if (queue_.size() >= max_)
	{
		LOGW("PUT : queue is full !!!");
		return false;
	}

	if (back)
		queue_.push_back(data);
	else queue_.push_front(data);

	sGet_.on();
	if ( queue_.size() < max_)
		sPut_.on();

	return true;
}

template <typename QDATA>
bool Queue<QDATA>::get(QDATA& data, UInt32 tm_wait, bool front)
{
	if (!sGet_.wait(tm_wait))
		return false;

	Lock lock(mutex_);
	if (queue_.empty())
	{
		LOGW("GET : queue is empty !!!");
		return false;
	}

	if (front)
	{
		data = queue_.front();
		queue_.pop_front();
	}
	else
	{
		data = queue_.back();
		queue_.pop_back();
	}

	if (queue_.size() < max_)
		sPut_.on();
	if (!queue_.empty())
		sGet_.on();

	return true;
}

template <typename QDATA>
void Queue<QDATA>::clear()
{
	Lock lock(mutex_);
	queue_.clear();
	sPut_.on();
}

#endif