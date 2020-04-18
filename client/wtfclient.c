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
#include <openssl/sha.h>

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



typedef struct ManifestRecord{
    char* projectName;
    char* version;
    char* file;
    unsigned char* hash;
} ManifestRecord;
typedef enum Boolean {true = 1, false = 0} Boolean; 

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

// SOCKET METHODS
// ==================================================================
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

/* blocking read */
char* block_read(int fd, int targetBytes){
    char* buffer = (char*) malloc(sizeof(char)*256);
    bzero(buffer,256);
    int status = 0;    
    int readIn = 0;
    do{
        status = read(fd, buffer+readIn, targetBytes-readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    if (readIn < 0) 
        printf("ERROR reading from socket");
    return buffer; 
}

/* blocking write */
void block_write(int fd, char* data, int targetBytes){
    int status = 0;    
    int readIn = 0;
    do{
        status = write(fd, data+readIn, targetBytes-readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    if (readIn < 0) 
        printf("ERROR writing to socket");
}
int read_len_message(int fd){
    //printf("um");
    char* buffer = (char*) malloc(sizeof(char)*50);
    bzero(buffer,50);
    int status = 0;    
    int readIn = 0;
    do{
        status = read(fd, buffer+readIn, 1);
        readIn +=status;
    } while (buffer[readIn-1] != ':' && status > 0);
    char* num = (char*) malloc(sizeof(char)*strlen(buffer));
    strncpy(num, buffer, strlen(buffer)-1);
    num[strlen(buffer)] = '\0';
    free(buffer);
    //printf("%s", num);
    int len = atoi(num);
    free(num);
    return len;
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
    block_write(sockfd,"48:Incoming client connection connection successful",51);
    int len = read_len_message(sockfd);
    char* buffer = block_read(sockfd, len);
    printf("%s\n",buffer);
    free(buffer);
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
    block_write(outputFile, hostname, strlen(hostname));
    block_write(outputFile, "\n", 1);
    block_write(outputFile, port, strlen(port));
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
            printf("Warning: %s file is empty.\n", file);
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
        free(fileData);
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

/*Sending command to create project in server.*/
int write_to_server(int sockfd, char* argv1, char* argv2){
    int nlen = strlen(argv2);
    int clen = nlen+digits(nlen)+strlen(argv1)+2;
    char* command = (char*)malloc(clen+1*sizeof(char)); 
    snprintf(command, clen+1, "%s:%d:%s", argv1, nlen, argv2);
    int n = write(sockfd, command, strlen(command));
    free(command);
    return n;
}

char* read_from_server(int sockfd){
    /*Read the project name sent from the server.*/
    char* buffer = (char*)malloc(256*sizeof(char));
    bzero(buffer,256);
    int n = read(sockfd,buffer,255);
    if(n < 0) printf("ERROR reading to socket.\n");
    printf("%s\n", buffer);
    return buffer;
}

/* Returns true if file exists in project */
Boolean fileExists(char* fileName){
    DIR *dir = opendir(fileName);
    if (dir != NULL){
        printf("Fatal Error: %s Not a file or file path\n", fileName);
        closedir(dir);
        return false;
    }
    closedir(dir);
    int fd = open(fileName, O_RDONLY);
    if(fd == -1){
        //printf("Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        printf("Fatal Error: %s does not exist\n", fileName);
        return false;
    }
    close(fd); 
    return true;
}
/* Returns true if project is in folder */
Boolean searchForProject(char* projectName){
    DIR *dir = opendir(projectName);
    if (dir == NULL){
        printf("Fatal Error: Project %s does not exist\n", projectName);
        return false;
    }
    closedir(dir);
    return true;
}
/* Returns a hex formatted hash */
unsigned char* getHash(char* data){;
    unsigned char digest[16];
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, data, strlen(data));
    SHA256_Final(digest, &context);
    char* hexHash = (char*) malloc(sizeof(char)*16);
    check_malloc_null(hexHash);
    sprintf(hexHash, "%02x", digest);
    printf("Hash: %s\n", hexHash);
    return hexHash;
}
/* Returns number of lines in file */
int number_of_lines(char* fileData){
    int count = 0;
    int pos = 0;
    while (pos < strlen(fileData)){
        if (fileData[pos] == '\n'){
            count++;
        }
        pos++;
    }
    return count;
}
/* Parses one line of the Manifest and addes to stuct */
void add_to_struct(char* line, ManifestRecord** manifest, int recordCount){
    int start = 0;
    int pos = 0;
    int count = 0;
    ManifestRecord* record = (ManifestRecord*) malloc(sizeof(ManifestRecord));
    while (pos < strlen(line)){
        if (line[pos] == ' ' || line[pos] == '\n'){
            int len = pos - start;
            char* temp = (char*) malloc(sizeof(char)*len+1);
            temp[0] = '\0';
            strncpy(temp, &line[start], len);
            temp[len] = '\0';
            switch (count){
                case 0:
                    record->version = temp;
                    break;
                case 1:
                    record->projectName = temp;
                    break;
                case 2:
                    record->file = temp;
                    break;
                case 3:
                    record->hash = temp;
                    count = -1;
                    break;
            }
            //printf("Word: %s\n", temp);
            count++;
            //printf("%d %d %d\n", start, pos, len);
            start = pos+1;
        }
        //printf("%c\n", line[pos]);
        pos++;
    }
    manifest[recordCount] = record;
}
/* Returns an array of Manifest Records */
ManifestRecord** create_manifest_struct(char* fileData){
    int start = 0;
    Boolean version = false;
    int pos = 0;
    int numberOfRecords = number_of_lines(fileData);
    //printf("Number of Records %d\n",numberOfRecords);
    ManifestRecord** manifest = (ManifestRecord**) malloc(sizeof(ManifestRecord*)*numberOfRecords);
    int recordCount = 0;
    while (pos < strlen(fileData)){
        if (fileData[pos] == '\n'){
            int len = pos - start;
            char* temp = (char*) malloc(sizeof(char)*len+2);
            temp[0] = '\0';
            strncpy(temp, &fileData[start], len+1);
            temp[len+1] = '\0';
            if (version){ // if version number has already been seen
                //printf("Line: %s\n", temp);
                add_to_struct(temp, manifest, recordCount);
                start = pos+1;
                recordCount++;
                free(temp);
            }
            else{
                ManifestRecord* record = (ManifestRecord*) malloc(sizeof(ManifestRecord));
                char* manifestCount = (char*) malloc(sizeof(char)*50);
                sprintf(manifestCount, "%d", numberOfRecords);
                record->projectName = NULL;
                record->version = temp;
                record->hash = manifestCount;
                record->file = NULL;
                manifest[recordCount] = record;
                version = true;
                start = pos+1;
                recordCount++;
            }
        }
        pos++;
    }
    return manifest;
}

/* Formats one manifest record */
char* printManifest(ManifestRecord* record){
    int len;

    if (record->projectName == NULL){
        return record->version;
    }
    else{
        int len = strlen(record->version)+strlen(record->projectName)+strlen(record->file)+strlen(record->hash) + 1 + 3;
        char* line = (char*) malloc(sizeof(char)*len);
        line[0] = '\0';
        strcat(line, record->version);
        strcat(line, " ");
        strcat(line,record->projectName );
        strcat(line, " ");
        strcat(line,record->file );
        strcat(line, " ");
        strcat(line,record->hash );
        return line;

    }
}
/* Returns size of manifest which is stored in the first position of the array hash value */
int getManifestStructSize(ManifestRecord** manifest){
    return atoi(manifest[0]->hash);
}

/* Look in the manifest for a particular file name formatted as project/filepath */
Boolean search_manifest(ManifestRecord** manifest, char* targetFile){
    int x = 1;
    int size = getManifestStructSize(manifest);
    while ( x < size){
        if (strcmp(manifest[x]->file, targetFile) == 0){
            return true;
        }
        x++;
    }
    return false;
}

/* Free Manifest Array */
void freeManifest(ManifestRecord** manifest){
    int size = getManifestStructSize(manifest);
    free(manifest[0]->version);
    free(manifest[0]->hash);
    free(manifest[0]);
    int x = 1;
    while (x < size){
        free(manifest[x]->projectName);
        free(manifest[x]->version);
        free(manifest[x]->file);
        free(manifest[x]->hash);
        free(manifest[x]);
        x++;
    }
    free(manifest);
}

// ACTION METHODS
// ============================================================================
Boolean add_file_to_manifest(char* projectName, char* fileName, char* manifestPath){
    int fd = open(manifestPath, O_WRONLY | O_APPEND);
    if(fd == -1){
        return false;
    }
    char* fileData = read_file(manifestPath);
    ManifestRecord** manifest = create_manifest_struct(fileData);
    if (search_manifest(manifest, fileName)){
        printf("Fatal Error: File %s already exists in Manifest\n", fileName);
    }
    else{
        write(fd, "1",1);
        write(fd, " ", 1);
        write(fd, projectName, strlen(projectName));
        write(fd, " ", 1);
        write(fd, fileName, strlen(fileName));
        write(fd, " ", 1);
        char* hashcode = getHash(fileData);
        write(fd, hashcode, strlen(hashcode));
        write(fd, "\n", 1);
        free(hashcode);
    }
    freeManifest(manifest);
    free(fileData);
    close(fd);
}
Boolean remove_file_from_manifest(char* projectName, char* fileName, char* manifestPath){
    char* fileData = read_file(manifestPath);
    ManifestRecord** manifest = create_manifest_struct(fileData);
    int x = 1;
    int size = getManifestStructSize(manifest);
    int fd = open(manifestPath, O_WRONLY | O_TRUNC);
    if(fd == -1){
        return false;
    }
    write(fd, manifest[0]->version, strlen(manifest[0]->version));
    Boolean remove = false;
    printf("%s\n", fileName);
    while ( x < size){
        if (strcmp(fileName, manifest[x]->file) != 0){
            char* temp = printManifest(manifest[x]);
            write(fd,temp, strlen(temp));
            write(fd,"\n", 1);
            //printf("%d", x);
            free(temp);
        }
        else {
            remove = true;
        }
        //printf("%s\n", manifest[x]->file);
        x++;
    }
    if (!remove){
        printf("Fatal Error: Manifest does not contain file\n");
        return false;
    }
    freeManifest(manifest);
    free(fileData);
    close(fd);
}
// MAIN METHOD  ================================================================================
int main(int argc, char** argv) {
    int sockfd;
    if (argc == 4 && strcmp(argv[1], "configure") == 0){
        write_configure(argv[2], argv[3]);
    }
    else if(argc == 3 && (strcmp(argv[1], "create")==0 || strcmp(argv[1], "checkout")==0)){
        sockfd = read_configure_and_connect();
        int n = write_to_server(sockfd, argv[1], argv[2]);
        if(n < 0)
            printf("ERROR writing to socket.\n");
        else{
            char* buffer = read_from_server(sockfd);
            parseBuffer_create(buffer);
        }
        /*disconnect server at the end!*/
        block_write(sockfd, "Done", 4);
        printf("Client Disconnecting");
        close(sockfd);

    }
    else if(argc == 3 && (strcmp(argv[1], "destroy")==0)){
        sockfd = read_configure_and_connect();
        int n = write_to_server(sockfd, argv[1], argv[2]);
        if(n < 0)
            printf("ERROR writing to socket.\n");
        else{
            char* buffer = read_from_server(sockfd);
        }
        /*disconnect server at the end!*/
        block_write(sockfd, "Done", 4);
        printf("Client Disconnecting");
        close(sockfd);
    }
    else if (argc == 4 && (strcmp(argv[1],"add") == 0 || strcmp(argv[1],"remove") == 0)){
        // combine project name and file path 
        char* filePath = (char*) malloc(strlen(argv[2])+strlen(argv[3])+2);
        check_malloc_null(filePath);
        strcpy(filePath, argv[2]);
        strcat(filePath, "/");
        strcat(filePath, argv[3]);

        if (searchForProject(argv[2])){
            /* create manifest file path */
            char* manifestFilePath = (char*) malloc(strlen(argv[2])+1+10);
            strcpy(manifestFilePath, argv[2]);
            strcat(manifestFilePath, "/.Manifest");

            /* if manifest exists */
            if (fileExists(manifestFilePath)){
                if (fileExists(filePath) && (strcmp(argv[1],"add") == 0)) {
                    add_file_to_manifest(argv[2], filePath, manifestFilePath);
                }
                else if (strcmp(argv[1],"remove") == 0){
                    remove_file_from_manifest(argv[2], filePath, manifestFilePath);
                }
            }
            free (manifestFilePath);
        }
        free(filePath);
    }
    else {
        printf("Fatal Error: Invalid Arguments\n");
    }
    sockfd = read_configure_and_connect();
    // code to disconnect let server socket know client socket is disconnecting
    // printf("Client Disconnecting");
    // write(sockfd, "Done",4);
    close(sockfd);
    return 0;
}

