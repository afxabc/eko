#ifndef _LOG_HEADER__
#define _LOG_HEADER__

#include "common.h"
#include "mutex.h"

class LogPrint;
class Log : boost::noncopyable
{
public:
	enum LEVEL
	{
		L_ERROR = 0,
		L_WARN,
		L_DEBUG,
		L_INFO,
		L_NONE,
	};

	static Log& defaultLog();

	inline void setLevel(LEVEL level) 
	{
		level_ = level;
	}

	inline bool checkLevel(LEVEL level) 
	{
		return level_ >= level;
	}

	inline void setPrint(LogPrint* print)
	{
		Lock lock(mutex_);
		print_.reset(print);
	}

	void print(LEVEL level, const char* sformat, ...);

protected:
	Log();

protected:
	LEVEL level_;
	boost::scoped_ptr<LogPrint> print_;
	Mutex mutex_;
};

class LogPrint
{
public:
	virtual void print(Log::LEVEL level, const char* sformat)
	{
		puts(sformat);
	}
};

extern Log& LOG_;

#define LOGPRINT(x) LOG_.setPrint((x))

#define LOG(x) LOG_.setLevel(Log::L_##x)

#define LOGI(...)	\
	if (LOG_.checkLevel(Log::L_INFO))	\
		LOG_.print(Log::L_INFO, __VA_ARGS__)

#define LOGD(...)	\
	if (LOG_.checkLevel(Log::L_DEBUG))	\
		LOG_.print(Log::L_DEBUG, __VA_ARGS__)

#define LOGW(...)	\
	if (LOG_.checkLevel(Log::L_WARN))	\
		LOG_.print(Log::L_WARN, __VA_ARGS__)

#define LOGE(...)	\
	if (LOG_.checkLevel(Log::L_ERROR))	\
		LOG_.print(Log::L_ERROR, __VA_ARGS__)

#endif
