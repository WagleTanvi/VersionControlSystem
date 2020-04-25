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

typedef enum Boolean{
    true = 1,
    false = 0
} Boolean;

//================== PROTOTYPES===========================================

void block_write(int fd, char *data, int targetBytes);

//==================== HELPER METHODS ================================
/*Count digits in a number*/
int digits(int n);

/*Returns a string converted number*/
char* to_Str(int number);

/* Check if malloc data is null */
void check_malloc_null(void *data);

/*Returns a substring*/
char* getSubstring(int bcount, char* buffer, int nlen);

/*Returns a substring (delete later)*/
char* substr(char *src, int m, int n);

/*Returns the length up to the colon*/
int getUnknownLen(int bcount, char* buffer);

/*Makes a directory with all the subdirectories etc.*/
void mkdir_recursive(const char *path);

/* Returns true if file exists in project */
Boolean fileExists(char* fileName);

/* Returns true if project is in folder */
Boolean searchForProject(char* projectName);

/* Method to non-blocking read a file and returns a string with contents of file */
char* read_file(char* file);

//================================= FREE METHODS==================================================================
/*Free 2d String array*/
void free_string_arr(char** arr, int size);

//================================= PRINTING METHODS==================================================================

/*Prits a 2d string array*/
void print_2d_arr(char** arr, int size);


#endif