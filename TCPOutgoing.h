#ifndef __tcpoutgoing__h
#define __tcpoutgoing__h

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <netinet/in.h>

using namespace std;

class TCPOutgoing {
	string IP;
	int sock;
	int port;

public:
	~TCPOutgoing();
	TCPOutgoing();
	TCPOutgoing(const TCPOutgoing& copy);
	TCPOutgoing(int sock_new, struct sockaddr_in* ip_address);
	void connect(const char* server, int port);
	ssize_t send(const char* buffer, size_t len);
	ssize_t receive(char* buffer, size_t len, int timeout = 0);
	string getIP();
	int getPort();

private:
	bool waitForRead(int timeout);
	int resolveHostName(const char* host, struct in_addr* addr);

};

#endif