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


/*ENCRYPTION*/

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// keys should be hidden
/*
 openssl enc -aes-192-cbc -k secret -P -md sha1
salt=577F1C618F640145
key=EE94A22BBAA41EC0B8317105C65C5169CA14171D43E650C6
iv =692E3E442B68A2E2A89D3D4DAC7A418D
*/

#define KEY "EE94A22BBAA41EC0B8317105C65C5169CA14171D43E650C6"
#define IV "692E3E442B68A2E2A89D3D4DAC7A418D"

#define PORT "3490"
#define OUTPUT_FILE_NAME "mesg.txt"

/* Deccryption Function */
int strdecrypt(unsigned char *inputstr, int inputstrlen, unsigned char *key, unsigned char *iv, unsigned char *decout) {
    int inlen, decoutlen;
    EVP_CIPHER_CTX *strdeccontext = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(strdeccontext, EVP_aes_192_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(strdeccontext, decout, &inlen, inputstr, inputstrlen);
    decoutlen = inlen;
    EVP_DecryptFinal_ex(strdeccontext, decout + inlen, &inlen);
    decoutlen += inlen;
    EVP_CIPHER_CTX_free(strdeccontext);
    return decoutlen;
}

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
    int sockfd; 
    ssize_t numbytes;  
    struct addrinfo connection; // Defines the connection type
    struct addrinfo *servinfo; // Contains structs for making connection
    struct addrinfo *connected; // Stores result of connection operation
    int status;

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname f/t\n");
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

    //freeaddrinfo(servinfo); // Dlete this struct

    if(*argv[2] == 'f'){
        char buffer[BUFSIZ];

        recv(sockfd, buffer, BUFSIZ-1, 0);
        int file_size = atoi(buffer);

        //printf("File size: %d\n", file_size);

        FILE *received_file = fopen(OUTPUT_FILE_NAME, "w");

        if(received_file == NULL){
            perror("File not received");
            return 1;
        }

        int remaining_data = file_size;

        //initializations
        unsigned char *key = (unsigned char *)KEY; //key pointer
        unsigned char *iv = (unsigned char *)IV; // IV pointer
        unsigned char *output = (unsigned char *)malloc(file_size);
        OpenSSL_add_all_algorithms(); //
        OPENSSL_config(NULL);   //

        while(((numbytes = recv(sockfd, (unsigned char*)buffer, BUFSIZ-1, 0) > 0) && (remaining_data > 0))) {
            
            //Decrypt the buffere here!
            
            /* Debugging */
            printf("Encrypted Input: \n");
            BIO_dump_fp (stdout, (const char *)buffer, file_size);
            /* Debugging */
         
            int outlen = strdecrypt((unsigned char *)buffer, file_size , key, iv, (unsigned char *)output);
            
            int test = fwrite(output, sizeof(char), outlen, received_file);
            printf("Bytes written: \n%d\n", test);
            //printf("size of buff %d", BUFSIZ-1);
            if(test < numbytes){
                perror("Write fail");
            }
            remaining_data -= numbytes;
            printf("Received");
        }
        fclose(received_file);

    }

    else if(*argv[2] == 't'){
        unsigned char buffer[BUFSIZ]; // Buffer for receiving sent data.

        if ((numbytes = recv(sockfd, buffer, BUFSIZ-1, 0)) == -1) {
            perror("Receive exceeds max data limit, modify MAXDATASIZE for larger messages.");
            return 1;
        }
        /* prep for encyption*/

        unsigned char *key = (unsigned char *)KEY; //key pointer
        unsigned char *iv = (unsigned char *)IV; // IV pointer
        //initializations
        OpenSSL_add_all_algorithms(); //
        OPENSSL_config(NULL);   //
        unsigned char output[BUFSIZ];
        
        int outlen = strdecrypt(buffer, numbytes, key, iv, output);

        output[outlen] = '\0';

        printf("client: received '%s'\n",output);

        close(sockfd);
    }
    return 0;
}
