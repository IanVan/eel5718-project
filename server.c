/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

 //
 /*ENCRYPTION*/

#include <openssl/conf.h>
#include <openssl/evp.h>
// #include <openssl/err.h>

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
#define BACKLOG 10     // how many pending connections queue will hold
#define FILE_NAME "test_msg.txt"

/* encryption function for string */

int strencrypt(unsigned char *inputstr, int inputstrlen, unsigned char *key, unsigned char *iv, unsigned char *encout){
    int inlen, encoutlen;
    EVP_CIPHER_CTX *strenccontext = EVP_CIPHER_CTX_new(); //setup context
    EVP_EncryptInit_ex(strenccontext, EVP_aes_192_cbc(), NULL, key, iv); // configure encryption options
    EVP_EncryptUpdate(strenccontext, encout, &inlen, inputstr, inputstrlen);
    encoutlen = inlen;
    EVP_EncryptFinal_ex(strenccontext, encout + inlen, &inlen);
    encoutlen += inlen;
    EVP_CIPHER_CTX_free(strenccontext);
    return encoutlen;
}

void sigchld_handler(int s)
{
    int saved_errno = errno; // Save errors which may be overwritten here.

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// Bind to first avaliable connection.
addrinfo* bind_connection(addrinfo *servinfo, int &sock){
    struct addrinfo *temp;
    int sock_set = 1;

    for(temp = servinfo; temp != NULL; temp = temp->ai_next) {
        if ((sock = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sock_set, sizeof(int)) == -1) {
            perror("setsockopt");
            return NULL;
        }

        if (bind(sock, temp->ai_addr, temp->ai_addrlen) == -1) {
            close(sock);
            perror("server: bind");
            continue;
        }

        return temp;
    }


}

void sendstring(int sock, unsigned char* string, int len){
    if (send(sock, string, len, 0) == -1){ //Transmit data, which is stored as a char pointer
        perror("send");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;  // Listen on sock_fd, new connection on new_fd
    int file;

    struct addrinfo connection; // Will define the connection type
    struct addrinfo *servinfo; // Contains structs for making connection - created by getaddrinfo()
    struct addrinfo *bound; // Will store the result of the bind operation.

    struct sockaddr_storage client_addr; // Connector address information
    socklen_t sin_size;

    char file_size[256];
    struct stat file_stats;

    if(argc < 2){
        fprintf(stderr, "usage: server message f/t\n");
        return 1;
    }

    memset(&connection, 0, sizeof connection);
    connection.ai_family = AF_INET; // Using IPv4
    connection.ai_socktype = SOCK_STREAM; // Connect using TCP 
    connection.ai_flags = AI_PASSIVE; // Automatically detect IP of system running server

    int status;
    if ((status = getaddrinfo(NULL, PORT, &connection, &servinfo)) != 0) { // Check to make sure addrinfo struct was initalized properly
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    bound = bind_connection(servinfo, sockfd);

    freeaddrinfo(servinfo); // Delete struct

    if((argc == 2) && (*argv[1] == 'f')){
        if((file = open(FILE_NAME, O_RDONLY)) == -1){
            perror("file open error");
            return 1;
        }

        if(fstat(file, &file_stats) == -1){
            perror("file stat");
            return 1;
        }
    }

    if (bound == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 1;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }


    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = sigchld_handler; // Points to a function used for deleting child processed to avoid zombie processes.

    sigemptyset(&act.sa_mask); // Makes sure no signals are blocked during execution of sigchld_handler.
    act.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        perror("Error deleting zombie processes\n"); // Clean up the zombies
        return 1;
    }

    printf("server: waiting for connection...\n");

    char ip[INET_ADDRSTRLEN];

    //Now send the rest of the file
    if((argc == 3) && (*argv[2] == 't')){
        
        /* prep for encyption*/

        unsigned char *key = (unsigned char *)KEY; //key pointer
        unsigned char *iv = (unsigned char *)IV; // IV pointer
        //initializations
        OpenSSL_add_all_algorithms(); //
        OPENSSL_config(NULL);   //
        
        // set up input to be encrypted
        unsigned char *inputstr = (unsigned char *)argv[1]; 
        // set up output to be sent to client
        unsigned char sendbuff[2048];
        int outlen = strencrypt(inputstr, strlen ((char *)inputstr), key, iv, sendbuff);
        printf("Encrypted string: \n");
        BIO_dump_fp (stdout, (const char *)sendbuff, outlen);

        while(1) {  // Accept connection
            sin_size = sizeof client_addr;
            new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
            if (new_fd == -1) {
                perror("accept connection failed");
                continue;
            }

            inet_ntop(client_addr.ss_family, &((struct sockaddr_in*)&client_addr)->sin_addr, ip, sizeof ip);
            printf("server: got connection from %s\n", ip);

            if (!fork()) { // This is the child process
                close(sockfd); // Child process does not need alistener
                sendstring(new_fd, sendbuff, outlen);
                close(new_fd); // Close socket after completion.
                return 0;
            }
            close(new_fd);
        }
    }
    else if((argc == 2) && (*argv[1] == 'f')){
        sprintf(file_size, "%d", (int)file_stats.st_size);
        //First we send the size of the file
        sin_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept connection failed");
            return 0;
        }

        inet_ntop(client_addr.ss_family, &((struct sockaddr_in*)&client_addr)->sin_addr, ip, sizeof ip);

        printf("server: got connection from %s\n", ip);
        
        off_t offset = 0;
        int sent = 0;
        int remaining_data = file_stats.st_size;
        printf("remaining_data = %d\n", remaining_data);
        //Open the file using read(), encrypt it, then write(), pretty sure that will work for passing it to the sendfile function.
        //Alternatively look into openssl like we talked about.

        unsigned char *key = (unsigned char *)KEY; //key pointer
        unsigned char *iv = (unsigned char *)IV; // IV pointer
        //initializations
        OpenSSL_add_all_algorithms(); //
        OPENSSL_config(NULL);   //
        // set up input to be encrypted

        unsigned char *inputstr = (unsigned char *)malloc(file_stats.st_size); //make a place to store the file
        unsigned char *sendbuff = (unsigned char *)malloc(file_stats.st_size+16);
        char name[] = "/tmp/fileXXXXXX";
        int out_fd =  mkstemp(name);//open("output", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        
        int size = read(file, (unsigned char *)inputstr, ((unsigned int)file_stats.st_size));

        //printf("nonEncrypted string: \n%s\n", inputstr);

        int outlen = strencrypt(inputstr, size, key, iv, sendbuff);

        sprintf(file_size, "%d", outlen);

        sent = send(new_fd, file_size, strlen(file_size), 0);
        if ( sent == -1){ //Transmit data, which is stored as a char pointer
            perror("sending file size");
        }
        
        //printf("File size: %d\n", atoi(file_size));
        sent = 0;

        int outsize = write(out_fd, (unsigned char *)sendbuff, outlen);

        
        /* Debugging */
        printf("Encrypted string: \n");
        BIO_dump_fp (stdout, (const char *)sendbuff, outsize);
        /* Debugging */
        remaining_data = outsize;
        while(((sent = sendfile(new_fd, out_fd, &offset, outsize)) > 0) && (remaining_data > 0)){
            //printf("Sent %d bytes, offset = %d, %d bytes left\n", sent, (int)offset, remaining_data);
            remaining_data -= sent;
            //sent = sendfile(new_fd, out_fd, &offset, outsize);
        }
        if(sent == -1){
            perror("Sendfile");
        }
        printf("Sent\n");
        //printf("remaining_data = %d\n", remaining_data);
        unlink(name);
        close(out_fd);
        close(sockfd); // Child process does not need alistener
        close(new_fd);

    }
    return 0;
}