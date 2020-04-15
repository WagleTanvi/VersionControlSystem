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
#include <libgen.h>


//====================HELPER METHODS================================
/*Count digits in a number*/
int digits(int n) {
    int count = 0;
    while (n != 0) {
        n /= 10;
        ++count;
    }
    return count;
}

/* Check if malloc data is null */
void check_malloc_null(void* data){
    if ((char*) data == NULL ){
        // malloc is null 
        printf("Could not allocate memory\n");
        exit(1);
    }
}

char* getSubstring(int bcount, char* buffer, int nlen){
    int count = 0;
    char* substr = (char*)malloc(nlen+1 * sizeof(char));
    while(count < nlen){
        substr[count]=buffer[bcount];
        count++;
        bcount++;
    } 
    substr[nlen] = '\0';
    return substr;
}

char* substr(char *src, int m, int n){
	int len = n - m;
	char *dest = (char*)malloc(sizeof(char) * (len + 1));
	int i = m;
    while(i < n && (*(src + i) != '\0')){
		*dest = *(src + i);
		dest++;
        i++;
	}
	*dest = '\0';
	return dest - len;
}


int getUnknownLen(int bcount, char* buffer){
    int count = 0;
    while(buffer[bcount]!=':'){
        bcount++;
        count++;
    }
    return count;
}

void mkdir_recursive(const char *path){
    char *subpath, *fullpath;
    fullpath = strdup(path);
    subpath = dirname(fullpath);
    if (strlen(subpath) > 1)
        mkdir_recursive(subpath);
    int n = mkdir(path, 0775);
    if(n < 0){
        if(errno != 17){
            printf("ERROR unable to make directory: %s\n", strerror(errno));
        }
    }
    free(fullpath);
}

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

//=========================== NETWORKING=============================

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

//================================CONFIGURE=============================
/* Create Configure File */
void write_configure(char* hostname, char* port){
    int outputFile = open("./client/.configure", O_WRONLY | O_CREAT | O_TRUNC, 00600); // creates a file if it does not already exist
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
        char* fileData = read_file("./client/.configure");
        if (fileData == NULL){
            printf("Fatal Error: Configure File not found.\n");
            return;
        }
        char* host = strtok(fileData, "\n");
        char* port = strtok(NULL, "\n"); 
        if (port != NULL && host != NULL){
            sockfd = create_socket(host, port);
        }
        //free(fileData);
        return sockfd;
}

//=================================CREATE==============================
//create:12:Pfirstproject:22:F./firstproject/.Manifest:0:C
void parseBuffer_create(char* buffer){
    int bcount = 0;
    int toklen = -1;
    char* tok = NULL;
    char* write_to_file = NULL;
    while(bcount < strlen(buffer)){
        if(toklen < 0){
            toklen = getUnknownLen(bcount, buffer);
        }
        tok = getSubstring(bcount, buffer, toklen); // "12"
        toklen = -1;
        if(isdigit(tok[0])){ //token is a number
            toklen = atoi(tok);
        } else {
            //create project
            if(tok[0] == 'P'){
                char* projectName = substr(tok, 1, strlen(tok));
                mkdir_recursive(projectName);
                int ch = chmod("./client", 0775);
                if(ch < 0) printf("ERROR setting perrmissions.\n");
                ch = chmod(projectName, 0775);
                if(ch < 0) printf("ERROR setting perrmissions: %s.\n", strerror(errno));
            }
            //create file
            else if(tok[0] == 'F'){
                char* filePath = substr(tok, 1, strlen(tok));
                write_to_file = filePath;
                int n = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if(n < 0) printf("ERROR making file.\n");                
            }
            //write data **need the previous file**
            else if(tok[0] == 'C'){
                char* fileContent = substr(tok, 1, strlen(tok));
                int fn = open(write_to_file, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if(fn < 0) printf("ERROR could not open file to write to.\n");
                int n = write(fn, fileContent, strlen(fileContent));
                if(n < 0) printf("ERROR writing data.\n");                
            }
        }
        bcount += strlen(tok);
        bcount++; //this is for the semicolon
    }    
    free(tok);
}

// MAIN METHOD  ================================================================================
int main(int argc, char** argv) {
    int sockfd;
    if (argc == 4 && strcmp(argv[1], "configure") == 0){
        write_configure(argv[2], argv[3]);
    }
    else if(argc == 3 && (strcmp(argv[1], "create")==0 || strcmp(argv[1], "checkout")==0)){
        sockfd = read_configure_and_connect();
        /*Sending command to create project in server.*/
        int nlen = strlen(argv[2]);
        int clen = nlen+digits(nlen)+strlen(argv[1])+2;
        char* command = (char*)malloc(clen+1*sizeof(char)); 
        snprintf(command, clen+1, "%s:%d:%s", argv[1], nlen, argv[2]); 
        int n = write(sockfd, command, clen+1);
        if(n < 0)
            printf("ERROR writing to socket.\n");
        else{
            /*Read the project name sent from the server.*/
            char buffer[256];
            bzero(buffer,256);
            n = read(sockfd,buffer,255);
            if(n < 0) printf("ERROR reading to socket.\n");
            printf("%s\n", buffer);
            parseBuffer_create(buffer);
        }
        n = write(sockfd, "Done", 4);
        free(command);
    }
    else {
        printf("Fatal Error: Invalid Arguments\n");
        //return 0;
    }

    // code to disconnect let server socket know client socket is disconnecting
    // printf("Client Disconnecting");
    // write(sockfd, "Done",4);

    close(sockfd);
    return 0;
}

