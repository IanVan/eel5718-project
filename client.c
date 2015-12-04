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

#define PORT "3490"

#define MAXDATASIZE 1024 // Max number of bytes that can be received

// Initializes the connection
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
    struct addrinfo connection; // Defines the connection type
    struct addrinfo *servinfo; // Contains structs for making connection
    struct addrinfo *connected; // Stores result of connection operation
    int status;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        return 1;
    }

    memset(&connection, 0, sizeof connection);
    connection.ai_family = AF_INET; // Using IPv4
    connection.ai_socktype = SOCK_STREAM; // TCP connection


    if ((status = getaddrinfo(argv[1], PORT, &connection, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    connected = create_connection(servinfo, sockfd); // Sets up the actual connection

    if (connected == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 1;
    }

    char ip[INET_ADDRSTRLEN];

    inet_ntop(connected->ai_family, ((struct sockaddr_in*)connected->ai_addr), ip, sizeof ip); // Create aan IP address
    printf("client: connecting to %s\n", ip);

    freeaddrinfo(servinfo); // Dlete this struct

    char buffer[MAXDATASIZE]; // Buffer for receiving sent data.

    if ((numbytes = recv(sockfd, buffer, MAXDATASIZE-1, 0)) == -1) {
        perror("Receive exceeds max data limit, modify MAXDATASIZE for larger messages.");
        return 1;
    }

    buffer[numbytes] = '\0';

    printf("client: received '%s'\n",buffer);

    close(sockfd);

    return 0;
}