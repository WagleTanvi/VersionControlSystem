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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

typedef struct Record
{
    char *version; //for manifest it is the version number, for upgrade and push it is the command 'M','A', or 'D'
    char *file;    //file path (includes the project name)
    unsigned char *hash;
} Record;

/*Create a struct that holds the project name and the mutex. Basically a hashmap that links project name with a mutex.*/
typedef struct MutexArray{
    char* projectName;
    pthread_mutex_t mutex;
} MutexArray;

typedef enum Boolean
{
    true = 1,
    false = 0
} Boolean;

//================== HELPER ===========================================
/*Count digits in a number*/
int digits(int n);

/*Returns a string converted number*/
char *to_Str(int number);

/* Check if malloc data is null */
void check_malloc_null(void *data);

/*Get substring of a string*/
char *getSubstring(int bcount, char *buffer, int nlen);

/*Makes directory and all subdirectories*/
void mkdir_recursive(const char *path);

/*Return string of file content*/
char *getFileContent(char *file, char *flag);

/*Returns if server has the given project - needs the full path.*/
Boolean search_proj_exists(char *project_name);

/* Returns number of lines in file */
int number_of_lines(char *fileData);

/*Returns the contents from the file requested from the client*/
char *fetch_file_from_client(char *fileName, int clientSoc);

/* tar extra credit */
void tarFile(char *file, char* dir);

void untarFile(char *file);

Boolean search_tars(char *tar);

char *extract_path(char *path);

/*Returns all the projects in the CWD*/
int find_all_projects();

/*Returns the corresponding mutex to the project name*/
pthread_mutex_t find_mutex(char* pname);

/*Returns a string to send to current version*/
char *current_version_format(char *req_dir);

int get_version_from_tar(char* project_name, char* tar_name);

void remove_new_versions(char* project_name, int req_version);

/*Given a project name, duplicate the directory*/
void duplicate_dir(char *project_path, const char *new_project_path, int history, int version);

/*Returns the names of all the  projects in the CWD as a string array*/
char** get_project_names(int size);

//=============================== HISTORY ======================
/*Saves each the commits made in each push to a log called .History*/
void save_history(char *commitData, char *projectName, char *version);

//=============================== CURRENT VERSION ======================
/*Returns the current version of a project as a string*/
char *get_current_version(char *buffer, int clientSoc, char flag);

//=============================== ROLLBACK ======================
//23:projectname:13
void rollback(char *buffer, int clientSoc);

//=============================== PUSH ======================
/*On fail, remove any pending commits.*/
void expire_pending_commits(char *project_name, char *good_commit);

//push:23:projectname:234:blahblahcommintcontent
void push_commits(char *buffer, int clientSoc);

//=============================== COMMIT ======================
void create_commit_file(char *buffer, int clientSoc);

//===============================GET MANIFEST======================
/*Sends the contents of the manifest to the client.*/
int send_manifest(char *project_name, int clientSoc);

/*Gets the project manifest that exists on server side.*/
void fetchServerManifest(char *buffer, int clientSoc);

/* Returns true if file exists in project */
Boolean fileExists(char *fileName);

void fetchFile(char *buffer, int clientSoc, char *command);

//=============================== DESTROY ======================
/*Returns 0 on success and -1 on fail on removing all files and directories given a path.*/
int remove_directory(char *dirPath);

/*Remove a project from the server side.*/
void destroyProject(char *buffer, int clientSoc);

//=============================== CHECKOUT ======================
/*Increments the global counter- cmd_count*/
void inc_command_length(char *dirPath);

/*Returns an char** array that stores all the things to be concatenated together.*/
char *checkoutProject(char *command, char *dirPath, int clientSoc);

//============================= CREATE ===================================
/*Create project folder.*/
void createProject(char *buffer, int clientSoc);

//=============================== READ CLIENT MESSAGE ======================
/*Parse through the client command.*/
void parseRead(char *buffer, int clientSoc);

//============================ NETWORKING ==================================
/* blocking read */
char *block_read(int fd, int targetBytes);

/* blocking write */
void block_write(int fd, char *data, int targetBytes);

int read_len_message(int fd);

/* This method disconnects from client if necessary in the future*/
void disconnectServer(int fd);

#endif