#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include "TCPOutgoing.h"

TCPOutgoing::~TCPOutgoing(){
	close(sock);
}

TCPOutgoing::TCPOutgoing(){
	sock = -1;
	port = 0;
	IP = "";
}

TCPOutgoing::TCPOutgoing(int sock_new, struct sockaddr_in* ip_address){
	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, (struct in_addr*)&(ip_address->sin_addr.s_addr), str, INET_ADDRSTRLEN);
	IP = str;
	//Not sure if I need ntohs here, double check
	port = ntohs(ip_address->sin_port); 
}

ssize_t TCPOutgoing::send(const char* buffer, size_t len){
	return write(sock, buffer, len);
}

ssize_t TCPOutgoing::receive(char* buffer, size_t len, int timeout){
	if(timeout <= 0)
		return read(sock, buffer, len);
	if (waitForRead(timeout)){
		return read(sock, buffer, len);
	}
	return -2;
}

void TCPOutgoing::connect(const char* server, int port){
	struct sockaddr_in address;

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if(resolveHostName(server, &(address.sin_addr)) != 0) {
		inet_pton(AF_INET, server, &(address.sin_addr));
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("socket failure");
		close(sock);
		return;
	}
	if(::connect(sock, (struct sockaddr*)&address, sizeof(address)) != 0) {
		perror("connection failed");
		close(sock);
	}

	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, (struct in_addr*)&(address.sin_addr.s_addr), str, INET_ADDRSTRLEN);
	IP = str;
	//Not sure if I need ntohs here, double check
	port = ntohs(address.sin_port); 
	return;
}

string TCPOutgoing::getIP(){
	return IP;
}

int TCPOutgoing::getPort(){
	return port;
}

bool TCPOutgoing::waitForRead(int timeout){
	//fd_set allows for listeing for inputs without blocking
	fd_set s_set;

	//tv contains a time in seconds and is a valid fd_set type
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	//zeros the set and then ads the socket to the set
	FD_ZERO(&s_set);
	FD_SET(sock, &s_set);
	//Waits for activity on the socket or until the timeout, tv
	if(select(sock + 1, &s_set, NULL, NULL, &tv) > 0){
		return true;
	}
	return false;

}

int TCPOutgoing::resolveHostName(const char* hostname, struct in_addr* addr){
	struct addrinfo *res;
	int result = getaddrinfo(hostname, NULL, NULL, &res);
	if(result == 0) {
		memcpy(addr, &((struct sockaddr_in *) res->ai_addr)->sin_addr, sizeof(struct in_addr));
		freeaddrinfo(res);
	}
	return result;
}