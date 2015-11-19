all: server.cpp
	g++ -std=c++11 server.cpp TCPOutgoing.cpp TCPOutgoing.h -o server

clean:
	$(RM) server
