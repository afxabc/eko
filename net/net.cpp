#include "base/log.h"
#include "net.h"

int socket_nonblock(SOCKET socket, int enable)
{
#ifdef WIN32
    u_long param = enable;
    return ioctlsocket(socket, FIONBIO, &param);
#else
	int param = fcntl(socket, F_GETFL);
	if (enable)
		param |= O_NONBLOCK;
	else param &= ~O_NONBLOCK;
	return fcntl(socket, F_SETFL, param);
#endif
}

#ifdef WIN32

int poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
    fd_set read_set;
    fd_set write_set;
    fd_set exception_set;
    nfds_t i;
    int n;
    int rc;

    if (numfds >= FD_SETSIZE) 
	{
		LOGE("poll error : numfds %d >= %d !", numfds, FD_SETSIZE);
        return -1;
    }

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&exception_set);

    n = 0;
    for (i = 0; i < numfds; i++) 
	{
        if (fds[i].fd == INVALID_SOCKET)
            continue;

        if (fds[i].events & POLLIN)
            FD_SET(fds[i].fd, &read_set);
        if (fds[i].events & POLLOUT)
            FD_SET(fds[i].fd, &write_set);
        if (fds[i].events & POLLERR)
            FD_SET(fds[i].fd, &exception_set);

        if (fds[i].fd >= n)
            n = fds[i].fd + 1;
    }

    if (n == 0)
        /* Hey!? Nothing to poll, in fact!!! */
        return 0;

	if (timeout >= 0) 
	{
        struct timeval tv;
        tv.tv_sec  = timeout / 1000;
        tv.tv_usec = 1000 * (timeout % 1000);
        rc = select(n, &read_set, &write_set, &exception_set, &tv);
    }
    else rc = select(n, &read_set, &write_set, &exception_set, NULL);

    if (rc < 0)
        return rc;

    for (i = 0; i < numfds; i++) 
	{
        fds[i].revents = 0;

        if (FD_ISSET(fds[i].fd, &read_set))
            fds[i].revents |= POLLIN;
        if (FD_ISSET(fds[i].fd, &write_set))
            fds[i].revents |= POLLOUT;
        if (FD_ISSET(fds[i].fd, &exception_set))
            fds[i].revents |= POLLERR;
    }

    return rc;
}

#endif

