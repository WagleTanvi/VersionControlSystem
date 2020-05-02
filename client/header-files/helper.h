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

void fetchFile(char *buffer, int sockfd);

void increment_manifest_version(char* project_name, int sockfd);

//================================= FREE METHODS==================================================================
/*Free 2d String array*/
void free_string_arr(char** arr, int size);

//================================= PRINTING METHODS==================================================================

/*Prits a 2d string array*/
void print_2d_arr(char** arr, int size);

//========================HASHING===================================================
unsigned char* getHash(char* s);

/*Returns an array holding the live hashes for each file in the given record*/
char **getLiveHashes(Record **record_arr, int size);

//=================================== COMMIT ===================================
/* Removes the commit file from the client after push is called! */
void remove_commit_file(int sockfd, char* project_name);

/*Sends the commit file to the server.*/
char* write_commit_file(int sockfd, char *project_name, char *server_record_data);

//=========================== UPGRADE METHODS==================================================================
char *fetch_file_from_server(char *fileName, int sockfd);

void write_record_to_file(int fd, Record **records, Boolean append, int size);

void add_file_to_manifest(char *hashS, char *versionS, char *fileS, Record **manifest, int count);

int find_number_of_adds(Record **records);

void upgrade(char *projectName, int sockfd);

//=========================== UPDATE METHODS==================================================================
void update(char *projectName, int sockfd);

//===================================== ADD/REMOVE METHODS ============================================================================
Boolean add_file_to_record(char *projectName, char *fileName, char *recordPath);

Boolean remove_file_from_record(char *projectName, char *fileName, char *recordPath);

//=================================================== CREATE ===============================================================
/*Creates the project in the client folder*/
void parseBuffer_create(char *buffer);

//=========================== SOCKET/CONFIGURE METHODS==================================================================
/* delay function - DOESNT really WORK*/
void delay(int number_of_seconds);

/* blocking write */
void block_write(int fd, char *data, int targetBytes);

/* blocking read */
char *block_read(int fd, int targetBytes);

/*Read the len of the message*/
int read_len_message(int fd);

/* Connect to Server*/
int create_socket(char *host, char *port);

/* Create Configure File */
void write_configure(char *hostname, char *port);

/* Method to read configure file (if exists) and calls create socket to connect to server*/
int read_configure_and_connect();

/*Sending command to create project in server. Returns the fd*/
void write_to_server(int sockfd, char* argv1, char* argv2, char* project_name);

/*Returns the buffer from the server*/
char *read_from_server(int sockfd);

#endif