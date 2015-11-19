#include <stdio.h>
#include <stdlib.h>
#include "TCPOutgoing.h"

int main(int argc, char** argv) {
	if(argc != 3){
		printf("usage: <port> <ip>\n");
		return 0;
	}

	int arg_port = atoi(argv[1]);
	char* ip = argv[2];
	int len;
	string message;
	char line[256];

	TCPOutgoing* stream = new TCPOutgoing();
	stream->connect(ip, arg_port);


}