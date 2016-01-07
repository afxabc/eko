#ifndef _LOG_HEADER__
#define _LOG_HEADER__

#include "common.h"
#include "mutex.h"


class Log : boost::noncopyable
{
public:
	enum LEVEL
	{
		L_ERROR = 0,
		L_WARN,
		L_INFO,
		L_DEBUG,
		L_NONE,
	};

	typedef boost::function<void(LEVEL, const char*)> LogPrint;

	static Log& defaultLog();

	inline void setLevel(LEVEL level) 
	{
		level_ = level;
	}

	inline bool checkLevel(LEVEL level) 
	{
		return level_ >= level;
	}

	template<class R>
	inline void setPrint(R r)
	{
		Lock lock(mutex_);
		print_ = boost::bind(r, _1, _2);
	}

	template<class R, class T>
	inline void setPrint(R r, T t)
	{
		Lock lock(mutex_);
		print_ = boost::bind(r, t, _1, _2);
	}

	inline void setPrint()
	{
		Lock lock(mutex_);
		print_ = defPrint_;
	}

	void print(LEVEL level, const char* sformat, ...);

private:
	Log();

private:
	LEVEL level_;
	LogPrint print_;
	LogPrint defPrint_;
	Mutex mutex_;
};


extern Log& LOG_;


inline void LOGPRINT()
{
	LOG_.setPrint();
}

template<class R>
inline void LOGPRINT(R r)
{
	LOG_.setPrint(r);
}

template<class R, class T>
inline void LOGPRINT(R r, T t)
{
	LOG_.setPrint(r, t);
}

#define LOGLEVEL(x) LOG_.setLevel(Log::L_##x)

#define LOGI(...)	\
	do	\
	{	\
		if (LOG_.checkLevel(Log::L_INFO))	\
			LOG_.print(Log::L_INFO, __VA_ARGS__);	\
	}while(0)

#define LOGD(...)	\
	do	\
	{	\
		if (LOG_.checkLevel(Log::L_DEBUG))	\
			LOG_.print(Log::L_DEBUG, __VA_ARGS__);	\
	}while(0)

#define LOGW(...)	\
	do	\
	{	\
		if (LOG_.checkLevel(Log::L_WARN))	\
			LOG_.print(Log::L_WARN, __VA_ARGS__);	\
	}while(0)

#define LOGE(...)	\
	do	\
	{	\
		if (LOG_.checkLevel(Log::L_ERROR))	\
			LOG_.print(Log::L_ERROR, __VA_ARGS__);	\
	}while(0)

#endif
