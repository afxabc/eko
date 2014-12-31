#ifndef NET_POLLER_H_
#define NET_POLLER_H_

#include "socket.h"
#include <map>
#include <vector>

class Poller : boost::noncopyable
{
public:
	bool updateSocket(const SocketPtr& socket);
	bool removeSocket(const SocketPtr& socket);

private:
	Mutex mutex_;

	typedef std::vector<pollfd> PollList;
	PollList list_;

	typedef std::map<SOCKET, SocketPtr> SocketMap;
	SocketMap sockets_;

};

#endif