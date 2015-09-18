#ifndef NET_INETADDRESS_H_
#define NET_INETADDRESS_H_

#include "net.h"

class InetAddress
{
public:
	InetAddress(UInt16 port = 0);
	InetAddress(const std::string& ip, UInt16 port);
	InetAddress(const struct sockaddr_in& addr) : sockaddr_(addr) {};
	InetAddress(const InetAddress& inet)
	{
		sockaddr_ = inet.sockaddr_;
	}

	~InetAddress(void);

	bool isNull() const 
	{
		return (sockaddr_.sin_port == 0); 
	}

	bool operator==(const InetAddress& iaddr) const
	{
		return (sockaddr_.sin_port==iaddr.sockaddr_.sin_port && sockaddr_.sin_addr.s_addr ==iaddr.sockaddr_.sin_addr.s_addr);
	}

	int getSize() const
	{ 
		return sizeof(sockaddr_); 
	}

	struct sockaddr_in& getSockaddrIn() const
	{ 
		return sockaddr_; 
	}
	
	struct sockaddr* getSockaddr() const
	{
		return (struct sockaddr*)(&sockaddr_);
	}

	UInt16 getPort() const
	{ 
		return ntohs(sockaddr_.sin_port); 
	}

	std::string getIP() const;

public:
	void update(UInt16 port);
	void update(const std::string& ip, UInt16 port);
	void update(const std::string& str);

	int toString(char* buf, int size) const;
	std::string toString() const;

private:
	mutable struct sockaddr_in sockaddr_;
};

inline std::ostream& operator<<(std::ostream& os, const InetAddress& inet)
{
	os << inet.toString();
	return os;
}

#endif
