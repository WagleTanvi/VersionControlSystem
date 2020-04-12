#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <time.h>

/* delay function - DOESNT really WORK*/ 
void delay(int number_of_seconds) 
{ 
    // Converting time into milli_seconds 
    int milli_seconds = 100000 * number_of_seconds; 
  
    // Storing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not achieved 
    while (clock() < start_time + milli_seconds) 
        ; 
} 
/* Connect to Server*/
int create_socket(char* host, char* port){
    int portno = atoi(port);
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // create socket and format hostname
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(host);
    if (server == NULL) {
        printf("ERROR, no such host\n");
        exit(0);
    }

    /* create struct*/
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Tries to connect to server every 3 seconds until successful */
    int status = 0;
    int count = 0;
    do {
        status = connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)); 
        if (status < 0){
            printf("Connection Attempt #%d\n", count);
        }
        delay(10);
        count++;
    } while (status < 0);

    /* Exchange initial connection messages*/
    int n = write(sockfd,"Incoming client connection connection successful",48);
    if (n < 0) 
         error("ERROR writing to socket");
    char buffer[256];
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    
    return sockfd;
}
/* Create Configure File */
void write_configure(char* hostname, char* port){
    int outputFile = open("./.configure", O_WRONLY | O_CREAT | O_TRUNC, 00600); // creates a file if it does not already exist
    if(outputFile == -1){
        printf("Fatal Error: %s\n", strerror(errno));
        close(outputFile);
        exit(1);
    }
    write(outputFile, hostname, strlen(hostname));
    write(outputFile, "\n", 1);
    write(outputFile, port, strlen(port));
    close(outputFile);

}
/* Check if malloc data is null */
void check_malloc_null(void* data){
    if ((char*) data == NULL ){
        // malloc is null 
        printf("Could not allocate memory\n");
        exit(1);
    }
}

/* Method to non-blocking read a file and returns a string with contents of file */
char* read_file(char* file){
    int fd = open(file, O_RDONLY);
    if(fd == -1){
        //printf("Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        return NULL;
    }
    struct stat stats;
    if(stat(file, &stats) == 0){
        int fileSize = stats.st_size; 
        if(fileSize == 0){
            printf("Warning: file is empty.\n");
        }
        char* buffer = (char*)malloc(fileSize+1 * sizeof(char));
        check_malloc_null(buffer);
        int status = 0;    
        int readIn = 0;
        do{
            status = read(fd, buffer+readIn, fileSize);
            readIn += status;
        } while (status > 0 && readIn < fileSize);
        buffer[fileSize] = '\0';
        close(fd);
        return buffer;
    }
    close(fd);
    printf("Warning: stat error. \n");
    return NULL;
}

/* Method to read configure file (if exists) and calls create socket to connect to server*/
int read_configure_and_connect(){
        int sockfd;
        char* fileData = read_file("./.configure");
        if (fileData == NULL){
            printf("Fatal Error: Configure File not found.\n");
            return;
        }
        char* host = strtok(fileData, "\n");
        char* port = strtok(NULL, "\n"); 
        if (port != NULL && host != NULL){
            sockfd = create_socket(host, port);
        }
        return sockfd;
}

// MAIN METHOD  ================================================================================
int main(int argc, char** argv) {
    int sockfd;
    if (argc == 4 && strcmp(argv[1], "configure") == 0){
        write_configure(argv[2], argv[3]);
    }
    else {
        printf("Fatal Error: Invalid Arguments\n");
        //return 0;
    }
    // this command reads configure file, connects to server, returns a socket 
    /* Comment this line out when you want to run configure */
    sockfd = read_configure_and_connect();

    // code to disconnect let server socket know client socket is disconnecting
    // printf("Client Disconnecting");
    // write(sockfd, "Done",4);

    close(sockfd);
    return 0;
}