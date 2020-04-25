#ifndef HELPER_H
#define HELPER_H

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
#include <openssl/sha.h>

typedef struct Record{
    char* version; //for manifest it is the version number, for upgrade and push it is the command 'M','A', or 'D'
    char* file; //file path (includes the project name)
    unsigned char* hash;
} Record;

typedef enum Boolean {true = 1, false = 0} Boolean;

//================== PROTOTYPES===========================================
void destroyProject(char* buffer, int clientSoc);
int remove_directory(char* dirPath);
void block_write(int fd, char* data, int targetBytes);
void duplicate_dir(char* project_path, char* new_project_path);

//================== HELPER ===========================================
/*Count digits in a number*/
int digits(int n);

/*Returns a string converted number*/
char* to_Str(int number);

/* Check if malloc data is null */
void check_malloc_null(void* data);

/*Get substring of a string*/
char* getSubstring(int bcount, char* buffer, int nlen);

/*Returns the command - create/checkout/etc...*/
char* getCommand(char* buffer);

/*Makes directory and all subdirectories*/
void mkdir_recursive(const char *path);

/*Returns swtiches the name from server to client*/
char* change_to_client(char* str);

/*Return string of file content*/
char* getFileContent(char* file, char* flag);

/*Returns if server has the given project - needs the full path.*/
Boolean search_proj_exists(char* project_name);

#endif