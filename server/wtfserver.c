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
#include <libgen.h>

int max_arr_size = 100;
int g_count = 0;

//================== HELPER METHODS========================================
/*Count digits in a number*/
int digits(int n) {
    int count = 0;
    while (n != 0) {
        n /= 10;     // n = n/10
        ++count;
    }
    return count;
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

/* Check if malloc data is null */
void check_malloc_null(void* data){
    if ((char*) data == NULL ){
        // malloc is null
        printf("Could not allocate memory\n");
        exit(1);
    }
}

char* getSubstring(int bcount, char* buffer, int nlen){
    char* substr = (char*)malloc(nlen * sizeof(char));
    check_malloc_null(substr);    
    int count = 0;
    while(count < nlen){
        substr[count]=buffer[bcount];
        count++;
        bcount++;
    }
    return substr;
}

char* getCommand(char* buffer){
    int strcount = 0;
    while(buffer[strcount]!=':'){
        strcount++;
    }
    char* command = (char*)malloc(strcount+1*sizeof(char));
    memset(command, ' ', strcount);
    int i = 0;
    while(i < strcount){
        command[i] = buffer[i];
        i++;
    }
    command[strcount+1] = '\0';
    return command;
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

//===============================CHECKOUT======================

char** inc_gcount(char** command){
    g_count++;
    if(g_count > max_arr_size){
        char** ext_command;
        ext_command = (char**)realloc(command, max_arr_size+100*sizeof(char*));
        max_arr_size += 100;
        return ext_command;
    }
    return command;
}

char* to_Str(int number){
    char* num_str = (char*)malloc(digits(number)+1*sizeof(char));
    snprintf(num_str, digits(number)+1, "%d", number);
    num_str[digits(number)+1] = '\0';
    return num_str;
}

char* change_to_client(char* str){
    char* new_str = (char*)malloc(strlen(str)+1*sizeof(char));
    strcpy(new_str, str);
    int i = 0;
    while(new_str[i] != '/') i++;
    char* path = substr(new_str, i, strlen(new_str));
    char* new_client_path = (char*)malloc((strlen(path)+7)*sizeof(char));
    snprintf(new_client_path, strlen(path)+7, "%s%s", "client", path);
    new_client_path[strlen(path)+7] = '\0';
    return new_client_path;
}

char* appendStr(char* str1, char* str2){
    char *tmp = strdup(str2);
    strcpy(str2, str1);
    strcat(str2, tmp);
    return str2;
}

char* make_command(char** commandArr){
    int max_length = 1000;
    int current_len = 0;
    char* command = (char*)malloc(max_length*sizeof(char));
    int i = 0;
    while(commandArr[i]!=NULL && i < max_arr_size){
        current_len += (strlen(commandArr[i])+1);
        if(current_len > max_length){
            max_length += 500;
            command = (char*)realloc(command, max_length*sizeof(char));
        }
        if(command != ""){
            strcat(command, ":");
        }
        strcat(command, commandArr[i]);
        i++;
    }
    return command;
}

char* getFileContent(char* file){
    int fd = open(file, O_RDONLY);
    if(fd == -1){
        printf("ERROR opening file: %s\n", strerror(errno));
        return NULL;
    }
    struct stat stats;
    if(stat(file, &stats) == 0){
        int fileSize = stats.st_size; 
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
        return appendStr("C", buffer);
    }
    close(fd);
    printf("Warning: stat error. \n");
    return NULL; 
}

char** checkoutProject(char** command, char* dirPath, int clientSoc){
    /*traverse the directory*/
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL){
        printf("ERROR opening directory.\n");
        return NULL;
    } else {
        char* dirPath_client = appendStr("P", change_to_client(dirPath));
        command[g_count] = to_Str(strlen(dirPath_client));
        command = inc_gcount(command);
        command[g_count] = dirPath_client;
        command = inc_gcount(command);
    }
    while ((d = readdir(dir)) != NULL) {
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);        
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0) continue;
        if (d->d_type != DT_DIR){
            char* path_client = appendStr("F./", change_to_client(path));
            command[g_count] = to_Str(strlen(path_client));
            command = inc_gcount(command);
            command[g_count] = path_client;
            command = inc_gcount(command);
            char* data_client = getFileContent(path);
            command[g_count] = to_Str(strlen(data_client));
            command = inc_gcount(command);
            command[g_count] = data_client;
            command = inc_gcount(command);
        }else{
            command = checkoutProject(command, path, clientSoc);
        }
    }

    closedir(dir);
    return command;
}

int search_proj_exists(char* project_name){    
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir("./server");
    if (dir == NULL){
        return 0;
    }    
    while ((d = readdir(dir)) != NULL) {
        if(strcmp(d->d_name, project_name)==0){
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

//=============================CREATE===================================
/*Create project folder.*/
void createProject(char* buffer, int clientSoc){
    /*parse through the buffer*/
    int bcount = 0;
    char* cmd = strtok(buffer, ":"); //"create"
    bcount += strlen(cmd)+1;
    char* plens = strtok(NULL, ":"); // "12"
    bcount += strlen(plens)+1;
    int pleni = atoi(plens); //12
    char* project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    char* pname_server = appendStr("server/", project_name);
    if(strcmp(cmd, "create")==0){
        /*check to see if project already exists on server*/
        if(foundProj==1){
            free(project_name);
            int n = write(clientSoc, "ERROR the project already exists on server.\n", 30);
            if(n < 0) printf("ERROR writing to the client.\n");
            return;
        }
        /*make project folder*/
        mkdir_recursive(pname_server);
        int ch = chmod("./server", 0775);
        if(ch<0) printf("ERROR set permission error.\n");
        ch = chmod(pname_server, 0775);
        if(ch<0) printf("ERROR set permission error.\n");
        /*make manifest file*/
        int pathLen = strlen(".Manifest")+strlen(pname_server);
        char* manifestPath = (char*)malloc(pathLen+4*sizeof(char));
        snprintf(manifestPath, pathLen+4, "./%s/%s", pname_server, ".Manifest");
        int manifestFile = open(manifestPath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if(manifestFile < 0){
            printf("ERROR unable to make manifestFile: %s\n", strerror(errno));
        }
        free(manifestPath);
    } else if(strcmp(cmd, "checkout")== 0){
        if(foundProj==0){
            free(project_name);
            int n = write(clientSoc, "ERROR project not in the server.\n", 36);
            if(n < 0) printf("ERROR writing to the client.\n");
            return;
        }
    }
    /*make the string to send to client*/
    char** command = (char**)malloc(max_arr_size*sizeof(char*));
    command[0] = cmd;
    g_count++;
    command = checkoutProject(command, pname_server, clientSoc);
    char* appendedCommand = make_command(command);
    /*write command to client*/
    int n = write(clientSoc, appendedCommand, strlen(appendedCommand));
    if(n < 0) printf("ERROR writing to the client.\n");
    /*free stuff*/
    free(pname_server);
    free(command);
    free(appendedCommand);
}

//===============================READ CLIENT MESSAGE======================
/*Parse through the client command.*/
void parseRead(char* buffer, int clientSoc){
    if(strstr(buffer, ":")!=0){
        //figure out the command
        char* command = getCommand(buffer);
        if(strcmp(command, "create")==0 || strcmp(command, "checkout")==0){
            createProject(buffer, clientSoc);
        }
        else {
            printf("Have not implemented this command yet!\n");
        }
        free(command);
    }
}

//============================NETWORKING==================================
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

// ===================  MAIN METHOD  ================================================================================
int main(int argc, char** argv) {
     int clientSock;
     int n;
     if (argc < 2) {
         printf("ERROR: No port provided\n");
         exit(1);
     }
    clientSock = set_up_connection(argv[1]);
     /* Code to infinite read from server and disconnect when client sends DONE*/
     while (1){
        char buffer[256];
        bzero(buffer,256);
        n = read(clientSock,buffer,255);
        buffer[n] = '\0';
        if (n < 0) printf("ERROR reading from socket");
        if (strcmp(buffer, "Done") == 0){
            printf("Client Disconnected");
            break;
        } else {
            /*Parse through the buffer*/
            parseRead(buffer, clientSock);
        }
     }

    close(clientSock);

    // things to consider: do a nonblocking read, disconnect server from client
     return 0;
}