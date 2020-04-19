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

typedef enum Boolean {true = 1, false = 0} Boolean;

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

/*Get substring of a string*/
// char* substr(char *src, int m, int n){
// 	int len = n - m;
// 	char *dest = (char*)malloc(sizeof(char) * (len + 1));
// 	int i = m;
//     while(i < n && (*(src + i) != '\0')){
// 		*dest = *(src + i);
// 		dest++;
//         i++;
// 	}
// 	*dest = '\0';
// 	return dest - len;
// }

/*Get substring of a string*/
char* getSubstring(int bcount, char* buffer, int nlen){
    char* bufferCopy = strdup(buffer);
    char* substr = (char*)malloc(nlen * sizeof(char));
    check_malloc_null(substr);    
    int count = 0;
    while(count < nlen){
        substr[count]=bufferCopy[bcount];
        count++;
        bcount++;
    }
    return substr;
}

/*Returns the command - create/checkout/etc...*/
char* getCommand(char* buffer){
    int strcount = 0;
    while(buffer[strcount]!=':'){
        strcount++;
    }
    char* command = (char*)malloc(strcount+1*sizeof(char));
    command[0] = '\0';
    int i = 0;
    while(i < strcount){
        command[i] = buffer[i];
        i++;
    }
    command[strcount+1] = '\0';
    return command;
}

/*Makes directory and all subdirectories*/
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

/*Returns an extended array.*/
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

/*Returns swtiches the name from server to client*/
char* change_to_client(char* str){
    char* project_name = getSubstring(7, str, strlen(str));
    char* pclient = (char*)malloc(8+strlen(project_name)*sizeof(char));
    char client[8] = "client/\0";
    int k = 0;
    while(k < 8){
        pclient[k] = client[k];
        k++;
    }
    strcat(pclient, project_name);
    return pclient;
    // char* new_str = (char*)malloc(strlen(str)+1*sizeof(char));
    // strdup(new_str, str);
    // int i = 0;
    // while(new_str[i] != '/') i++;
    // char* path = substr(new_str, i, strlen(new_str));
    // char* new_client_path = (char*)malloc((strlen(path)+7)*sizeof(char));
    // snprintf(new_client_path, strlen(path)+7, "%s%s", "client", path);
    // new_client_path[strlen(path)+7] = '\0';
    // return new_client_path;
}

/*Delete later.. this is bad*/
// char* appendStr(char* str1, char* str2){
//     char *tmp = strdup(str2);
//     strcpy(str2, str1);
//     strcat(str2, tmp);
//     return str2;
// }

/*Returns the string protocol given a list of all the things to concat together.*/
// char* make_command(char** commandArr){
//     int max_length = 1000;
//     int current_len = 0;
//     char* command = (char*)malloc(max_length*sizeof(char));
//     command[0] = '\0';
//     int i = 0;
//     while(i < g_count){
//         current_len += (strlen(commandArr[i])+1);
//         if(current_len > max_length){
//             max_length += 500;
//             command = (char*)realloc(command, max_length*sizeof(char));
//         }
//         if(command != ""){
//             strcat(command, ":");
//         }
//         strcat(command, commandArr[i]);
//         i++;
//     }
//     int length = strlen(command);

//     return command;
// }

/*Return string of file content*/
char* getFileContent(char* file, char* flag){
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

        /*Append the necessary flag!*/
        if(strcmp(flag, "")==0) 
            return buffer;
        else{
            char f[2] = "C\0";
            char* new_buffer = (char*)malloc(strlen(buffer)+2*sizeof(char));
            check_malloc_null(new_buffer);
            int i = 0;
            while(i < 2){
                new_buffer[i] = f[i];
                i++;
            }
            strcat(new_buffer, buffer);
            return new_buffer;
        }
    }
    close(fd);
    printf("Warning: stat error. \n");
    return NULL; 
}

/*Returns if server has the given project - needs the full path.*/
Boolean search_proj_exists(char* project_name){    
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir("./server");
    if (dir == NULL){
        return 0;
    }    
    while ((d = readdir(dir)) != NULL) {
        if(strcmp(d->d_name, project_name)==0){
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

//===============================GET MANIFEST======================
/*Sends the contents of the manifest to the client.*/
int send_manifest(char* project_name, int clientSoc){    
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(project_name);
    if (dir == NULL){
        return -1;
    }
    while ((d = readdir(dir)) != NULL) {
        snprintf(path, 4096, "%s/%s", project_name, d->d_name);        
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0) continue;
        if (d->d_type == DT_DIR){
            int result = send_manifest(path, clientSoc);
        }else{
            if(strcmp(d->d_name, ".Manifest")==0){
                /*add the length of the full commmand to the front of the protocol*/
                char* manifest_data = getFileContent(path, "");
                int m_len = strlen(manifest_data);
                char* m_len_str = to_Str(m_len);
                char* new_mdata = (char*)malloc(m_len+strlen(m_len_str)+1*sizeof(char));
                new_mdata[0] = '\0';
                strcat(new_mdata, m_len_str);
                strcat(new_mdata, ":");
                strcat(new_mdata, manifest_data);

                /*write to the client*/
                int n = write(clientSoc, new_mdata, strlen(new_mdata));
                if(n < 0){
                    printf("ERROR writing to client.\n");
                    return -1;
                }
                else return 0;
            }
        }
    }
    closedir(dir);
    return -2;
}

/*Gets the project manifest that exists on server side.*/
void fetchServerManifest(char* buffer, int clientSoc){
    /*parse through the buffer*/
    int bcount = 0;
    char* cmd = strtok(buffer, ":");
    bcount += strlen(cmd)+1;
    char* plens = strtok(NULL, ":");
    bcount += strlen(plens)+1;
    int pleni = atoi(plens);
    char* project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    /*Error checking*/
    if(foundProj==0){
        free(project_name);
        int n = write(clientSoc, "ERROR the project does not exist on server.\n", 40);
        if(n < 0) printf("ERROR writing to the client.\n");
        return;
    }

    /*Call method to write to the client socket.*/
    char* pserver = (char*)malloc(10+strlen(project_name));
    char server[10] = "./server/\0";
    int k = 0;
    while(k < 10){
        pserver[k] = server[k];
        k++;
    }
    int s = send_manifest(pserver, clientSoc);

    /*More error checking*/
    if(s == -1){
        printf("ERROR sending Manifest to client./\n");
    } else if(s == -2){
        printf("ERROR cannot find Manifest file./\n");
    }

    free(pserver);
}
//===============================DESTROY======================
/*Returns 0 on success and -1 on fail on removing all files and directories given a path.*/
int remove_directory(char* dirPath){
    int r = -1;
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL){
        printf("ERROR this is not a directory.\n");
        return -1;
    }
    r=0;
    while (!r && (d = readdir(dir)) != NULL) {
        int r2 = -1;
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);        
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0) continue;
        if (d->d_type == DT_DIR){
            r2 = remove_directory(path);
        }else{
            r2 = unlink(path);
        }
        r = r2;
    }
    closedir(dir);
    if (!r){
       r = rmdir(dirPath);
    }
    return 0;  
}

/*Remove a project from the server side.*/
void destroyProject(char* buffer, int clientSoc){
    int bcount = 0;
    char* cmd = strtok(buffer, ":");
    bcount += strlen(cmd)+1;
    char* plens = strtok(NULL, ":"); // "12"
    bcount += strlen(plens)+1;
    int pleni = atoi(plens); //12
    char* project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    
    /*Error checking*/
    if(foundProj==0){
        free(project_name);
        int n = write(clientSoc, "ERROR the project does not exist on server.\n", 40);
        if(n < 0) printf("ERROR writing to the client.\n");
        return;
    }

    /*Call remove_directory*/
    char* pserver = (char*)malloc(8+strlen(project_name));
    char server[8] = "server/\0";
    int k = 0;
    while(k < 8){
        pserver[k] = server[k];
        k++;
    } 
    int r = remove_directory(pserver);

    /*Send message to client*/
    if(r<0) printf("ERROR traversing directory.\n");
    else {
        free(project_name);
        int n = write(clientSoc, "Successfully destroyed project.\n", 33);
        if(n < 0) printf("ERROR writing to the client.\n");
        return;
    }

    free(pserver);
}

//===============================CHECKOUT======================
/*Returns an char** array that stores all the things to be concatenated together.*/
char* checkoutProject(char* command, char* dirPath, int clientSoc){
    /*traverse the directory*/
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL){
        printf("ERROR opening directory.\n");
        return NULL;
    } else {
        char* dirPath_client = change_to_client(dirPath);
        char f[2] = "P\0";
        char* new_dirpath = (char*)malloc(strlen(dirPath_client)+2*sizeof(char));
        check_malloc_null(dirPath_client);
        int i = 0;
        while(i < 2){
            new_dirpath[i] = f[i];
            i++;
        }
        strcat(new_dirpath, dirPath_client);
        char* length_of_path = to_Str(strlen(new_dirpath));
        strcat(command,length_of_path);
        strcat(command, new_dirpath);
    }
    while ((d = readdir(dir)) != NULL) {
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);        
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0) continue;
        if (d->d_type != DT_DIR){
            /*for a file*/
            char* path_client = change_to_client(path);
            char f[4] = "P./\0";
            char* new_path = (char*)malloc(strlen(path_client)+4*sizeof(char));
            check_malloc_null(new_path);
            int i = 0;
            while(i < 4){
                new_path[i] = f[i];
                i++;
            }
            strcat(new_path, path_client);
            char* length_of_path = to_Str(strlen(new_path));
            strcat(command, length_of_path);
            strcat(command, new_path);

            /*for the file content*/
            char* data_client = getFileContent(path, "C");
            char* length_of_data = to_Str(strlen(data_client));
            strcat(command, length_of_data);
            strcat(command, data_client);
        }else{
            command = checkoutProject(command, path, clientSoc);
        }
    }

    closedir(dir);
    return command;
}

//=============================CREATE===================================
/*Create project folder.*/
void createProject(char* buffer, int clientSoc){
    /*parse through the buffer*/
    int bcount = 0;
    char* cmd = strtok(buffer, ":");
    bcount += strlen(cmd)+1;
    char* plens = strtok(NULL, ":");
    bcount += strlen(plens)+1;
    int pleni = atoi(plens);
    char* project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    /*create project in server*/
    if(strcmp(cmd, "create")==0){
        /*check to see if project already exists on server*/
        if(foundProj==1){
            free(project_name);
            int n = write(clientSoc, "ERROR the project already exists on server.\n", 30);
            if(n < 0) printf("ERROR writing to the client.\n");
            return;
        }

        /*make project folder and set permissions*/
        char* pserver = (char*)malloc(8+strlen(project_name)+strlen("/.Manifest"));
        char server[8] = "server/\0";
        int k = 0;
        while(k < 8){
            pserver[k] = server[k];
            k++;
        } 
        mkdir_recursive(pserver);
        int ch = chmod("./server", 0775);
        if(ch<0) printf("ERROR set permission error.\n");
        ch = chmod(pserver, 0775);
        if(ch<0) printf("ERROR set permission error.\n");

        /*make manifest file*/
        strcat(pserver, "/.Manifest");
        int manifestFile = open(pserver, O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if(manifestFile < 0){
            printf("ERROR unable to make manifestFile: %s\n", strerror(errno));
        }
    } 
    else if(strcmp(cmd, "checkout")== 0){
        if(foundProj==0){
            free(project_name);
            int n = write(clientSoc, "ERROR project not in the server.\n", 36);
            if(n < 0) printf("ERROR writing to the client.\n");
            return;
        }
    }
    /*send string to client to make project*/    
    char* pserver = (char*)malloc(8+strlen(project_name));
    char server[8] = "server/\0";
    int k = 0;
    while(k < 8){
        pserver[k] = server[k];
        k++;
    }
    char* command = (char*)malloc(max_arr_size*sizeof(char));
    command[0] = '\0';
    strcat(command, cmd);
    command = checkoutProject(command, pserver, clientSoc);

    /*add the length of the full commmand to the front of the protocol*/
    int cmd_length = strlen(command);
    char* cmd_len_str = to_Str(cmd_length);
    char* new_command = (char*)malloc(strlen(cmd_len_str)+cmd_length+1*sizeof(char));
    new_command[0] = '\0';
    strcat(new_command, cmd_len_str);
    strcat(new_command, ":");
    strcat(new_command, command);

    /*write command to client*/
    int n = write(clientSoc, new_command, strlen(new_command));
    if(n < 0) printf("ERROR writing to the client.\n");

    /*free stuff*/
    free(command);
    free(new_command);
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
        else if(strcmp(command, "destroy")==0){
            destroyProject(buffer, clientSoc);
        }
        else if(strcmp(command, "commit")==0){
            fetchServerManifest(buffer, clientSoc);
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