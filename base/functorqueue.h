#ifndef _BASE_FUNCTOR_QUEUE_H__
#define _BASE_FUNCTOR_QUEUE_H__ 

#include "timestamp.h"
#include "mutex.h"
#include "common.h"

#include <map>
#include <vector>


template <typename T>
class Param
{
public:
	Param() : data_(NULL) {}
	Param(const T& data) : data_(new T(data)) {}

	Param(const Param& param) : data_(NULL)
	{ swap(param); }

	~Param()
	{ delete data_; }

	Param& operator=(const Param& param)
	{
		swap(param);
		return *this;
	}

	operator T()
	{ return *data_; }

private:
	void swap(const Param& param)
	{
		T* tmp = data_;
		data_ = param.data_;
		param.data_ = tmp;
	}

	mutable T* data_;
};

template <typename T>
inline Param<T> P(const T& data)
{ 
	return Param<T>(data); 
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

	void post(const Functor& func);						  //to actQueue_������ִ��
	UInt32 post(const Functor& func, MicroSecond delay); //delay<=0 to actQueue_��delay>0 to tmQueue_
														  //����sequence��cancelʱʹ��
	//����ִ�����е��ں���
	//���أ�
	//MicroSecond=0 : �޺���
	//MicroSecond>0 : δ���ڣ�����ʱ��ms��
	MicroSecond run();

	bool cancel(UInt32 sequence);

	void clear();

	bool empty()
	{ return (tmQueue_.empty() && actQueue_.empty()); }

	size_t size()
	{ return (tmQueue_.size()+actQueue_.size()); }

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
