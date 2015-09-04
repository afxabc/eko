
#ifndef NET_NET_H__
#define NET_NET_H__

#include "base/common.h"

#ifdef WIN32	/*windows*/

typedef int socklen_t;

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

#ifndef S_IRUSR
#define S_IRUSR S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR S_IWRITE
#endif

struct pollfd {
    int fd;
    short events;  /* events to look for */
    short revents; /* events that occurred */
};
/* events & revents */
#define POLLIN     0x0001  /* any readable data available */
#define POLLOUT    0x0002  /* file descriptor is writeable */
#define POLLRDNORM POLLIN
#define POLLWRNORM POLLOUT
#define POLLRDBAND 0x0008  /* priority readable data */
#define POLLWRBAND 0x0010  /* priority data can be written */
#define POLLPRI    0x0020  /* high priority readable data */

/* revents only */
#define POLLERR    0x0004  /* errors pending */
#define POLLHUP    0x0080  /* disconnected */
#define POLLNVAL   0x1000  /* invalid file descriptor */

typedef SOCKET FD;
typedef int nfds_t;
int poll(struct pollfd *fds, nfds_t numfds, int timeout);
#define closefd(s) closesocket((s))

#else	/*linux*/

#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fd.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

typedef int FD;
#define INVALID_SOCKET -1
#define closefd(s) close((s))

#endif

int fd_nonblock(FD fd, int enable);

#endif
