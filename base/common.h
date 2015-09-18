#ifndef BASE_COMMON_H_
#define BASE_COMMON_H_


#include <stdio.h>
#include <fcntl.h>

#ifdef WIN32

#include <io.h>
#include <winsock2.h>
#include <windows.h>

typedef unsigned __int64 UInt64;
typedef unsigned int UInt32;
typedef unsigned short UInt16;

#define snprintf _snprintf

inline int error_n()
{
	return GetLastError();
}


#else

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/time.h>

typedef unsigned long long UInt64;
typedef unsigned char BYTE;
typedef unsigned int UInt32;
typedef unsigned short UInt16;

inline int error_n()
{
	return errno;
}

#endif


#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

//boost¿â
#define BOOST_LIB_DIAGNOSTIC
#include <boost/noncopyable.hpp>
#include <boost/any.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>


typedef boost::function<void(void)> Functor;

#endif
