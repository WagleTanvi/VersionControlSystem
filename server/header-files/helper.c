#include "helper.h"
#include "record.h"

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
char* getSubstring(int bcount, char* buffer, int nlen){
    char* substr = (char*)malloc(nlen* sizeof(char));
    check_malloc_null(substr);    
    int count = 0;
    while(count < nlen){
        substr[count]=buffer[bcount];
        count++;
        bcount++;
    }
    substr[nlen] = '\0';
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
}

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
    DIR *dir = opendir("./");
    if (dir == NULL){
        return false;
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