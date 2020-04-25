#include "helper.h"
#include "record.h"

//================== PROTOTYPES===========================================

void block_write(int fd, char *data, int targetBytes);

//==================== HELPER METHODS ================================
/*Count digits in a number*/
int digits(int n)
{
    int count = 0;
    while (n != 0)
    {
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
void check_malloc_null(void *data)
{
    if ((char *)data == NULL)
    {
        // malloc is null
        printf("Could not allocate memory\n");
        exit(1);
    }
}

/*Returns a substring*/
char* getSubstring(int bcount, char* buffer, int nlen){
    int count = 0;
    char *substr = (char *)malloc(nlen + 1 * sizeof(char));
    while (count < nlen)
    {
        substr[count] = buffer[bcount];
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
    while (buffer[bcount] != ':')
    {
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
    if (n < 0)
    {
        if (errno != 17)
        {
            printf("ERROR unable to make directory: %s\n", strerror(errno));
        }
    }
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

//================================= FREE METHODS==================================================================
/*Free 2d String array*/
void free_string_arr(char** arr, int size){
    int i = 0;
    while(i < size){
        free(arr[i]);
        i++;
    }
    free(arr);
}
//================================= PRINTING METHODS==================================================================

/*Prints out 2d string array*/
void print_2d_arr(char** arr, int size){
    int i = 1;
    while(i < size){
        printf("%s\n", (arr[i]));
        i++;
    }
}