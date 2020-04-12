#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>
#include <netinet/in.h> 
#include <netdb.h>

int set_up_connection(char* port){
    int sockfd, clientSoc;
    struct sockaddr_in serv_addr, cli_addr;
    /* Set up Listening Socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        printf("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    int portno = atoi(port);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
            printf("ERROR on binding");
    listen(sockfd,5);

    /* Accept Client Connection*/
    int clilen = sizeof(cli_addr);
    clientSoc = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (clientSoc < 0) 
          printf("ERROR on accept");

    /* Exchange Initial Messages */
    char buffer[256];
    bzero(buffer,256);
    int n = read(clientSoc,buffer,255);
    if (n < 0) printf("ERROR reading from socket");
    printf("%s\n",buffer);
    n = write(clientSoc,"Client successfully connected to Server!",40);
    if (n < 0) printf("ERROR writing to socket");
    return clientSoc;
}

/* This method disconnects from server if necessary in the future*/
void disconnectServer(fd){
    write(fd, "Done",4);
    close(fd);
}

// =============================================================   MAIN METHOD  ================================================================================
int main(int argc, char** argv) {
     int clientSock; 
     int n;
     if (argc < 2) {
         printf("ERROR: No port provided\n");
         exit(1);
     }
    clientSock = set_up_connection(argv[1]);
     /* Code to infinite read from server and disconnect when client sends DONE*/
    //  while 1:
    //     bzero(buffer,256);
    //     n = read(newsockfd,buffer,255);
    //     buffer[n] = '\0';
    //     if (n < 0) printf("ERROR reading from socket");
    //     if (strcmp(buffer, "Done") == 0){
    //         printf("Client Disconnected");
    //         break;
    //     }
    close(clientSock);

    // things to consider: do a nonblocking read, disconnect server from client 
     return 0; 
}