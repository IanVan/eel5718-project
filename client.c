#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

addrinfo* create_connection(addrinfo *servinfo, int &sock){
    addrinfo* temp;

    for(temp = servinfo; temp != NULL; temp = temp->ai_next) {
        if ((sock = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sock, temp->ai_addr, temp->ai_addrlen) == -1) {
            close(sock);
            perror("client: connect");
            continue;
        }

        return temp;
    }

}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buffer[MAXDATASIZE];
    struct addrinfo connection;
    struct addrinfo *servinfo;
    struct addrinfo *connected;
    int status;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        return 1;
    }

    memset(&connection, 0, sizeof connection);
    connection.ai_family = AF_UNSPEC;
    connection.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], PORT, &connection, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    connected = create_connection(servinfo, sockfd);

    if (connected == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 1;
    }

    char ip[INET_ADDRSTRLEN];

    inet_ntop(connected->ai_family, ((struct sockaddr_in*)connected->ai_addr), ip, sizeof ip);
    printf("client: connecting to %s\n", ip);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buffer, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        return 1;
    }

    buffer[numbytes] = '\0';

    printf("client: received '%s'\n",buffer);

    close(sockfd);

    return 0;
}