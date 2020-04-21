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

typedef struct ManifestRecord{
    char* projectName;
    char* version;
    char* file;
    unsigned char* hash;
} ManifestRecord;

typedef enum Boolean {true = 1, false = 0} Boolean;

//====================HELPER METHODS================================
/*Returns the number of  digits in an int*/
int digits(int n) {
    int count = 0;
    while (n != 0) {
        n /= 10;
        ++count;
    }
    return count;
}

/*Returns a string converted number*/
char* to_Str(int number){
    char* num_str = (char*)malloc(digits(number)+1*sizeof(char));
    snprintf(num_str, digits(number)+1, "%d", number);
    num_str[digits(number)+1] = '\0';
    return num_str;
}

/* Check if malloc data is null */
void check_malloc_null(void* data){
    if ((char*) data == NULL ){
        // malloc is null 
        printf("Could not allocate memory\n");
        exit(1);
    }
}

/*Returns a substring*/
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

/*Returns a substring (delete later)*/
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

/*Returns the length up to the colon*/
int getUnknownLen(int bcount, char* buffer){
    int count = 0;
    while(buffer[bcount]!=':'){
        bcount++;
        count++;
    }
    return count;
}

/*Makes a directory with all the subdirectories etc.*/
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

/*Sending command to create project in server. Returns the fd*/
int write_to_server(int sockfd, char* argv1, char* argv2){
    int clen = strlen(argv2)+digits(strlen(argv2))+strlen(argv1)+2;
    char* command = (char*)malloc(clen+1*sizeof(char)); 
    snprintf(command, clen+1, "%s:%d:%s", argv1, strlen(argv2), argv2);
    char* new_command = (char*)malloc(strlen(command)+digits(strlen(command))+1 * sizeof(char));
    new_command[0] = '\0';
    strcat(new_command, to_Str((strlen(command))));
    strcat(new_command, ":");
    strcat(new_command, command);
    int n = write(sockfd, new_command, strlen(new_command));
    free(command);
    free(new_command);
    return n;
}

/*Returns the buffer from the server*/
char* read_from_server(int sockfd){
    /*Read the project name sent from the server.*/
    int len = read_len_message(sockfd);
    char* buffer = block_read(sockfd, len);
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

// FREE METHODS==================================================================
/*Free 2d String array*/
void free_string_arr(char** arr, int size){
    int i = 0;
    while(i < size){
        free(arr[i]);
        i++;
    }
    free(arr);
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

// PRINTING METHODS==================================================================
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

void print_2d_arr(char** arr, int size){
    int i = 1;
    while(i < size){
        printf("%s\n", (arr[i]));
        i++;
    }
}
// SOCKET METHODS==================================================================
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
    int len = atoi(num)-strlen(num);
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
    int outputFile = open("./client/.configure", O_WRONLY | O_CREAT | O_TRUNC, 00600); // creates a file if it does not already exist
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
    free(fileData);
    return sockfd;
}

//=================================CREATE==============================
/*Creates the project in the client folder*/
void parseBuffer_create(char* buffer){
    int bcount = 0;
    int toklen = -1;
    char* tok = NULL;
    char* write_to_file = NULL;
    int first_num_len = getUnknownLen(bcount, buffer);
    tok = getSubstring(bcount, buffer, first_num_len);
    bcount += strlen(tok);
    bcount++;
    while(bcount < strlen(buffer)){
        if(toklen < 0){
            toklen = getUnknownLen(bcount, buffer);
        }
        tok = getSubstring(bcount, buffer, toklen);
        toklen = -1;
        if(isdigit(tok[0])){ //token is a number
            toklen = atoi(tok);
        } else {
            /*create project*/
            if(tok[0] == 'P'){
                char* projectName = substr(tok, 1, strlen(tok));
                mkdir_recursive(projectName);
                int ch = chmod("./client", 0775);
                if(ch < 0) printf("ERROR setting perrmissions.\n");
                ch = chmod(projectName, 0775);
                if(ch < 0) printf("ERROR setting perrmissions: %s.\n", strerror(errno));
            }
            /*create file*/
            else if(tok[0] == 'F'){
                char* filePath = substr(tok, 1, strlen(tok));
                write_to_file = filePath;
                int n = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if(n < 0) printf("ERROR making file.\n");                
            }
            /*write datainto file */
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

//========================HASHING===================================================
/* Returns a hex formatted hash */
// unsigned char* getHash(char* data){
//     unsigned char digest[16];
//     SHA256_CTX context;
//     SHA256_Init(&context);
//     SHA256_Update(&context, data, strlen(data));
//     SHA256_Final(digest, &context);
//     char* hexHash = (char*) malloc(sizeof(char)*16);
//     check_malloc_null(hexHash);
//     sprintf(hexHash, "%02x", digest);
//     printf("Hash: %s\n", hexHash);
//     return hexHash;
// }
unsigned char* getHash(char* s){
	unsigned char *d = SHA256(s, strlen(s), 0);
 
	int i;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
		printf("%02x", d[i]);
	putchar('\n');
    return d;
}

/*Returns an array holding the live hashes for each file in the given manifest*/
char** getLiveHashes(ManifestRecord** manifest, int size){
    char** live_hash_arr = (char**)malloc(size*sizeof(char*));
    int i = 1;
    while(i < size){
        char* file_path = (char*)malloc(10+strlen(manifest[i]->file) * sizeof(char));
        char client[10] = "./client/\0";
        int k = 0;
        while(k < 10){
            file_path[k] = client[k];
            k++;
        }
        strcat(file_path, manifest[i]->file);
        char* file_content = read_file(file_path);
        char* live_hash =  getHash(file_content);
        live_hash_arr[i] = live_hash;
        i++;
    }
    return live_hash_arr;
}

// MANIFEST METHODS
// ============================================================================
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

//===================================COMMIT CHANGES===================================

char* append_client(int size){
    char* pclient = (char*)malloc(size * sizeof(char));
    char client[10] = "./client/\0";
    int k = 0;
    while(k < 10){
        pclient[k] = client[k];
        k++;
    }
}

/*Sends the commit file to the server.*/
void write_commit_file(int sockfd, char* project_name, char* server_manifest_data){
    //first check if the client has updates to be made
    char* pclient = append_client(10+strlen(project_name)+strlen("./Update"));
    strcat(pclient, project_name);
    strcat(pclient, "/.Update");
    if(fileExists(pclient)==true){
        if(strcmp(read_file(pclient),"")!=0){
            free(pclient);
            printf("ERROR update file not empty. Please update project!.\n");
            return;
        }
    }
    // free(pclient);

    /*check if there is a conflict file*/
    char* pclient2 = append_client(10+strlen(project_name)+strlen("./Conflict"));
    strcat(pclient2, project_name);
    strcat(pclient2, "/.Conflict");
    if(fileExists(pclient2)==true){
        free(pclient2);
        printf("ERROR conflicts still exist. Please resolve conflicts first.\n");
        return;
    }
    // free(pclient2);

    // /*get server and client manifest files*/
    char* pclient3 = append_client(10+strlen(project_name)+strlen("./Manifest"));
    strcat(pclient3, project_name);
    strcat(pclient3, "/.Manifest");
    ManifestRecord** server_manifest = create_manifest_struct(server_manifest_data);
    char* client_manifest_data = read_file(pclient3);
    ManifestRecord** client_manifest = create_manifest_struct(client_manifest_data);
    int client_manifest_size = getManifestStructSize(client_manifest);
    int server_manifest_size = getManifestStructSize(server_manifest);
    
    /*print out the manifests*/
    int x = 1;
    while (x < client_manifest_size){
        char* temp = printManifest(client_manifest[x]);
        printf("%s\n", temp);
        x++;
    }
    x = 1;
    while (x < server_manifest_size){
        char* temp = printManifest(server_manifest[x]);
        printf("%s\n", temp);
        x++;
    }

    /*compare manifset versions - throw warning if bad*/
    if(strcmp(server_manifest[0]->version, client_manifest[0]->version )!=0 ){
        printf("ERROR client needs to update manifest.\n");
        free(client_manifest_data);
        freeManifest(server_manifest);
        freeManifest(client_manifest);
        return;
    }

    /*compute an array of live hashes*/
    char** live_hash_arr = getLiveHashes(client_manifest, client_manifest_size);

    /*create path for commit*/
    char* pclient4 = append_client(10+strlen(project_name));
    strcat(pclient4, project_name);
    int path_len = strlen("./Commit")+strlen(pclient4);
    char* commit_path = (char*)malloc(path_len+1*sizeof(char));
    snprintf(commit_path, path_len+1, "%s%s", pclient4, "/.Commit");
    commit_path[path_len+1] = '\0'; 
    int commit_fd = open(commit_path, O_WRONLY | O_CREAT | O_TRUNC, 00600);

    /*go through each file in client manifest and compare manifests and write commit file*/
    int i = 1;
    while(i < client_manifest_size){
        /*check to see if they modified the code*/
        char* file_name = client_manifest[i]->file;
        Boolean found_file = search_manifest(server_manifest, file_name);
        Boolean write_file = false;
        if(found_file == true){
            if(strcmp(live_hash_arr[i], client_manifest[i]->hash)!=0){
                /*check that the client file is up to date*/
                int server_file_version = atoi(server_manifest[i]->version);
                int client_file_version = atoi(client_manifest[i]->version);
                if(server_file_version < client_file_version){
                    free(client_manifest_data);
                    freeManifest(server_manifest);
                    freeManifest(client_manifest);
                    close(commit_fd);
                    unlink(commit_path); //delete /Commit file
                    printf("ERROR client needs to synch repository with server before committing.\n");
                    return;
                }

                /*write that client modified the file*/
                //char* file_version = increment_file_version(client_manifest[i]->version); //where is the file version being incremented (seems like the manifest)
                write(commit_fd, "M", 1);
                write_file = true;
            }
        }
        else {
            /*write that client added a file*/
            write(commit_fd, "A", 1);          
            write_file = true;
        }
        if(write_file == true){
            write(commit_fd, " ", 1);
            write(commit_fd, file_name, strlen(file_name));
            write(commit_fd, " ", 1);
            write(commit_fd, client_manifest[i]->hash, strlen(client_manifest[i]->hash));
            write(commit_fd, "\n", 1);
        }
        i++;
    }
    i = 1;
    while(i < server_manifest_size){
        char* file_name = server_manifest[i]->file;
        Boolean found_file = search_manifest(client_manifest, file_name);
        if(found_file == false){
            /*write that client deleted a file*/
            write(commit_fd, "D", 1);          
            write(commit_fd, " ", 1);
            write(commit_fd, file_name, strlen(file_name));
            write(commit_fd, " ", 1);
            write(commit_fd, client_manifest[i]->hash, strlen(client_manifest[i]->hash));
        }
        i++;
    }

    /*Send the commit file to the server*/
    char* commit_file_content = read_file(commit_path);
    char* length_of_commit = to_Str(strlen(commit_file_content));
    char* send_commit_to_server = (char*)malloc(strlen("commit")+digits(project_name)+strlen(project_name)+digits(length_of_commit)+strlen(commit_file_content)+4 * sizeof(char));
    send_commit_to_server[0] = '\0';
    strcat(send_commit_to_server, "commit");
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, to_Str(strlen(project_name)));
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, project_name)
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, length_of_commit)
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, commit_file_content);
    char* extended_commit_cmd = (char*)malloc(digits(strlen(send_commit_to_server)+strlen(send_commit_to_server)+1*sizeof(char)));
    extended_commit_cmd[0] = '\0';
    strcat(extended_commit_cmd, to_str(strlen(send_commit_to_server)+1));
    strcat(extended_commit_cmd, ":");
    block_write(sockfd, extended_commit_cmd, strlen(extended_commit_cmd));

    /*Finally free!*/
    free(commit_path);
    freeManifest(server_manifest);
    freeManifest(client_manifest);
    free_string_arr(live_hash_arr, client_manifest_size);
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
    else if(argc == 3 && (strcmp(argv[1], "commit")==0)){
        sockfd = read_configure_and_connect();
        int n = write_to_server(sockfd, "manifest", argv[2]);
        if(n < 0)
            printf("ERROR writing to socket.\n");
        else{
            char* buffer = read_from_server(sockfd);
            write_commit_file(sockfd, argv[2], buffer);
        } 
    }
    else if(argc == 3 && (strcmp(argv[1], "push")==0)){
    
    }
    else {
        printf("Fatal Error: Invalid Arguments\n");
    }
    /*disconnect server at the end!*/
    // int n = write(sockfd, "Done", 4);
    // if(n < 0) printf("ERROR reading to socket.\n");

    // sockfd = read_configure_and_connect();
    // code to disconnect let server socket know client socket is disconnecting
    // printf("Client Disconnecting");
    // write(sockfd, "Done",4);
    // close(sockfd);
    return 0;
}

