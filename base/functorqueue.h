#ifndef _BASE_FUNCTOR_QUEUE_H__
#define _BASE_FUNCTOR_QUEUE_H__ 

#include "timestamp.h"
#include "mutex.h"
#include "common.h"

#include <map>
#include <vector>


template <typename T>
class Parcel
{
public:
	Parcel() : data_(NULL) {}
	Parcel(const T& data) : data_(new T(data)) {}

	Parcel(const Parcel& parcel) : data_(NULL)
	{ swap(parcel); }

	~Parcel()
	{ delete data_; }

	Parcel& operator=(const Parcel& parcel)
	{
		swap(parcel);
		return *this;
	}

	operator T()
	{ return *data_; }

private:
	void swap(const Parcel& parcel)
	{
		T* tmp = data_;
		data_ = parcel.data_;
		parcel.data_ = tmp;
	}

	mutable T* data_;
};

template <typename T>
inline Parcel<T> P(const T& data)
{ 
	return Parcel<T>(data); 
}

////////////////////////////////////////////////

class FunctorQueue
{
	class TmFunctor
	{
	public:
		TmFunctor(UInt32 seq, const Functor& func) 
			: sequence_(seq), func_(func) {}
	
		UInt32 sequence_;
		mutable Functor func_;

		friend class FunctorQueue;
	};

public:
	typedef std::multimap<Timestamp, TmFunctor> TimerQueue;
	typedef std::map<UInt32, TimerQueue::iterator> SequenceMap;
	typedef std::vector<Functor> ActiveQueue;
	static const UInt32 NULL_SEQUENCE = 0;

	FunctorQueue() : sequence_(NULL_SEQUENCE) {}

	void post(const Functor& func);						  //to actQueue_，立即执行
	UInt32 post(const Functor& func, MicroSecond delay); //delay<=0 to actQueue_，delay>0 to tmQueue_
														  //返回sequence，cancel时使用
	//立即执行所有到期函数
	//返回：
	//MicroSecond=0 : 无函数
	//MicroSecond>0 : 未到期，返回时间差（ms）
	MicroSecond run();

	bool cancel(UInt32 sequence);

	void clear();

	bool empty()
	{ return (tmQueue_.empty() && actQueue_.empty()); }

	size_t size()
	{ return (tmQueue_.size()+actQueue_.size()); }

	size_t actSize()
	{ return actQueue_.size(); }

	//debug
	void dump();
private:
	Mutex mutex_;
	TimerQueue tmQueue_;
	SequenceMap seqMap_;
	ActiveQueue actQueue_;
	UInt32 sequence_;
};

#endif
