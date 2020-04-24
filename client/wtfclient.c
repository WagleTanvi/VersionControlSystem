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
#include <errno.h>
/*So.. idk if you like this formatting method but basically it goes

helper
free
printing
hashing
records
commit
upgrade
update
add/remove
create
socket/networking
main

i kind of tried to follow the way of the asst.pdf

*/

typedef struct Record
{
    char *version; //for manifest it is the version number, for upgrade and push it is the command 'M','A', or 'D'
    char *file;    //file path (includes the project name)
    unsigned char *hash;
} Record;

typedef enum Boolean
{
    true = 1,
    false = 0
} Boolean;

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
char *to_Str(int number)
{
    char *num_str = (char *)malloc(digits(number) + 1 * sizeof(char));
    snprintf(num_str, digits(number) + 1, "%d", number);
    num_str[digits(number)] = '\0';
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
char *getSubstring(int bcount, char *buffer, int nlen)
{
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
char *substr(char *src, int m, int n)
{
    int len = n - m;
    char *dest = (char *)malloc(sizeof(char) * (len + 1));
    int i = m;
    while (i < n && (*(src + i) != '\0'))
    {
        *dest = *(src + i);
        dest++;
        i++;
    }
    *dest = '\0';
    return dest - len;
}

/*Returns the length up to the colon*/
int getUnknownLen(int bcount, char *buffer)
{
    int count = 0;
    while (buffer[bcount] != ':')
    {
        bcount++;
        count++;
    }
    return count;
}

/*Makes a directory with all the subdirectories etc.*/
void mkdir_recursive(const char *path)
{
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
    free(fullpath);
}

/* Returns true if file exists in project */
Boolean fileExists(char *fileName)
{
    DIR *dir = opendir(fileName);
    if (dir != NULL)
    {
        printf("Fatal Error: %s Not a file or file path\n", fileName);
        closedir(dir);
        return false;
    }
    closedir(dir);
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        //printf("Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        printf("Fatal Error: %s does not exist\n", fileName);
        return false;
    }
    close(fd);
    return true;
}

/* Returns true if project is in folder */
Boolean searchForProject(char *projectName)
{
    DIR *dir = opendir(projectName);
    if (dir == NULL)
    {
        printf("Fatal Error: Project %s does not exist\n", projectName);
        return false;
    }
    closedir(dir);
    return true;
}

/* Method to non-blocking read a file and returns a string with contents of file */
char *read_file(char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        //printf("Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        return NULL;
    }
    struct stat stats;
    if (stat(file, &stats) == 0)
    {
        int fileSize = stats.st_size;
        if (fileSize == 0)
        {
            printf("Warning: %s file is empty.\n", file);
        }
        char *buffer = (char *)malloc(fileSize + 1 * sizeof(char));
        check_malloc_null(buffer);
        int status = 0;
        int readIn = 0;
        do
        {
            status = read(fd, buffer + readIn, fileSize);
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
void free_string_arr(char **arr, int size)
{
    int i = 0;
    while (i < size)
    {
        free(arr[i]);
        i++;
    }
    free(arr);
}

/* Free Record Array */
// u = update/commmit/conflict file
// m = manifest file
// a = appended records
void freeRecord(Record **record_arr, char flag, int size)
{
    int s = size;
    int x = 0;
    if (flag == 'm')
    {
        free(record_arr[0]->version);
    }
    if (flag == 'm' || flag == 'u')
    {
        free(record_arr[0]->hash);
        if (record_arr[0] != NULL)
        {
            free(record_arr[0]);
        }
        x = 1;
    }
    while (x < s)
    {
        if (record_arr[x] != NULL)
        {
            free(record_arr[x]->version);
            free(record_arr[x]->file);
            free(record_arr[x]->hash);
            free(record_arr[x]);
        }
        x++;
    }
    if (record_arr != NULL)
    {
        free(record_arr);
    }
}

//================================= PRINTING METHODS==================================================================
/* Formats one record */
char *printRecord(Record *record)
{
    int len;
    if (record == NULL)
    {
        return NULL;
    }
    else
    {
        int len = strlen(record->version) + strlen(record->file) + strlen(record->hash) + 1 + 3;
        char *line = (char *)malloc(sizeof(char) * len);
        line[0] = '\0';
        strcat(line, record->version);
        strcat(line, " ");
        strcat(line, record->file);
        strcat(line, " ");
        strcat(line, record->hash);
        return line;
    }
}
char *printAllRecords(Record **record)
{
    printf("%s\n", record[0]->hash);
    int size = getRecordStructSize(record);
    int x = 1;
    while (x < size)
    {
        printf("%s\n", printRecord(record[x]));
        x++;
    }
}

void print_2d_arr(char **arr, int size)
{
    int i = 1;
    while (i < size)
    {
        printf("%s\n", (arr[i]));
        i++;
    }
}

//========================HASHING===================================================
unsigned char *getHash(char *data)
{
    unsigned char *digest = SHA256(data, strlen(data), 0);
    unsigned char *hexHash = (char *)malloc(sizeof(char) * 65);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hexHash + (i * 2), "%02x", digest[i]);
    }
    //printf("%s\n", hexHash);
    return hexHash;
}

/*Returns an array holding the live hashes for each file in the given record*/
char **getLiveHashes(Record **record_arr, int size)
{
    char **live_hash_arr = (char **)malloc(size * sizeof(char *));
    int i = 1;
    while (i < size)
    {
        char *file_path = (char *)malloc(10 + strlen(record_arr[i]->file) * sizeof(char));
        char client[10] = "./client/\0";
        int k = 0;
        while (k < 10)
        {
            file_path[k] = client[k];
            k++;
        }
        strcat(file_path, record_arr[i]->file);
        char *file_content = read_file(file_path);
        char *live_hash = getHash(file_content);
        live_hash_arr[i] = live_hash;
        i++;
    }
    return live_hash_arr;
}

// =======================  RECORD METHODS =====================================================
/* Returns number of lines in file */
int number_of_lines(char *fileData)
{
    int count = 0;
    int pos = 0;
    while (pos < strlen(fileData))
    {
        if (fileData[pos] == '\n')
        {
            count++;
        }
        pos++;
    }
    return count;
}

/* Parses one line of the Record and addes to stuct */
void add_to_struct(char *line, Record **record_arr, int recordCount)
{
    int start = 0;
    int pos = 0;
    int count = 0;
    Record *record = (Record *)malloc(sizeof(Record));
    while (pos < strlen(line))
    {
        if (line[pos] == ' ' || line[pos] == '\n')
        {
            int len = pos - start;
            char *temp = (char *)malloc(sizeof(char) * len + 1);
            temp[0] = '\0';
            strncpy(temp, &line[start], len);
            temp[len] = '\0';
            switch (count)
            {
            case 0:
                record->version = temp;
                break;
            case 1:
                record->file = temp;
                break;
            case 2:
                record->hash = temp;
                count = -1;
                break;
            }
            count++;
            start = pos + 1;
        }
        pos++;
    }
    record_arr[recordCount] = record;
}

/* Returns an array of records */
Record **create_record_struct(char *fileData, Boolean manifest)
{
    int start = 0;
    Boolean version = !manifest;
    int pos = 0;
    int numberOfRecords = number_of_lines(fileData);
    Record **record_arr = (Record **)malloc(sizeof(Record *) * numberOfRecords);
    int recordCount = 0;
    if (!manifest) // if it is an update/conflict/commit file
    {
        Record *record = (Record *)malloc(sizeof(Record));
        numberOfRecords++;
        record->hash = to_Str(numberOfRecords);
        record_arr[0] = record;
        recordCount += 1;
    }
    while (pos < strlen(fileData))
    {
        if (fileData[pos] == '\n')
        {
            // printf("%d\n", pos);
            int len = pos - start;
            char *temp = (char *)malloc(sizeof(char) * len + 2);
            temp[0] = '\0';
            strncpy(temp, &fileData[start], len + 1);
            temp[len + 1] = '\0';
            if (version)
            { // if version number has already been seen
                //printf("%s\n", temp);
                add_to_struct(temp, record_arr, recordCount);
                start = pos + 1;
                recordCount++;
                free(temp);
            }
            else
            {
                Record *record = (Record *)malloc(sizeof(Record));
                char *rec_count = (char *)malloc(sizeof(char) * 50);
                sprintf(rec_count, "%d", numberOfRecords);
                record->version = temp;
                record->file = NULL;
                record->hash = rec_count; //the hash stores the number of records!
                record_arr[recordCount] = record;
                version = true;
                start = pos + 1;
                recordCount++;
            }
        }
        pos++;
    }
    return record_arr;
}

/* Returns size of record array which is stored in the first position of the array hash value */
int getRecordStructSize(Record **record_arr)
{
    return atoi(record_arr[0]->hash);
}
/* Look in the records for a particular file name formatted as project/filepath */
Boolean search_Record(Record **record_arr, char *targetFile)
{
    int x = 1;
    int size = getRecordStructSize(record_arr);
    while (x < size)
    {
        if (strcmp(record_arr[x]->file, targetFile) == 0)
        {
            return true;
        }
        x++;
    }
    return false;
}

int search_record_hash(Record **record_arr, char *targetFile)
{
    int x = 1;
    int size = getRecordStructSize(record_arr);
    while (x < size)
    {
        if (strcmp(record_arr[x]->file, targetFile) == 0)
        {
            return x;
        }
        x++;
    }
    return -1;
}

//=================================== COMMIT ===================================

char *append_client(int size)
{
    char *pclient = (char *)malloc(size * sizeof(char));
    char client[10] = "./client/\0";
    int k = 0;
    while (k < 10)
    {
        pclient[k] = client[k];
        k++;
    }
}

/*Sends the commit file to the server.*/
void write_commit_file(int sockfd, char *project_name, char *server_record_data)
{
    //first check if the client has updates to be made
    char *pclient = append_client(10 + strlen(project_name) + strlen("./Update"));
    strcat(pclient, project_name);
    strcat(pclient, "/.Update");
    if (fileExists(pclient) == true)
    {
        if (strcmp(read_file(pclient), "") != 0)
        {
            free(pclient);
            printf("ERROR update file not empty. Please update project!.\n");
            return;
        }
    }
    // free(pclient);

    /*check if there is a conflict file*/
    char *pclient2 = append_client(10 + strlen(project_name) + strlen("./Conflict"));
    strcat(pclient2, project_name);
    strcat(pclient2, "/.Conflict");
    if (fileExists(pclient2) == true)
    {
        free(pclient2);
        printf("ERROR conflicts still exist. Please resolve conflicts first.\n");
        return;
    }
    // free(pclient2);

    // /*get server and client records*/
    char *pclient3 = append_client(10 + strlen(project_name) + strlen("./Manifest"));
    strcat(pclient3, project_name);
    strcat(pclient3, "/.Manifest");
    Record **server_manifest = create_record_struct(server_record_data, true);
    char *client_manifest_data = read_file(pclient3);
    Record **client_manifest = create_record_struct(client_manifest_data, true);
    int client_manifest_size = getRecordStructSize(client_manifest);
    int server_manifest_size = getRecordStructSize(server_manifest);

    /*print out the manifests*/
    int x = 1;
    while (x < client_manifest_size)
    {
        char *temp = printRecord(client_manifest[x]);
        printf("%s\n", temp);
        x++;
    }
    x = 1;
    while (x < server_manifest_size)
    {
        char *temp = printRecord(server_manifest[x]);
        printf("%s\n", temp);
        x++;
    }

    /*compare manifset versions - throw warning if bad*/
    if (strcmp(server_manifest[0]->version, client_manifest[0]->version) != 0)
    {
        printf("ERROR client needs to update manifest.\n");
        free(client_manifest_data);
        freeRecord(server_manifest, 'm', getRecordStructSize(server_manifest));
        freeRecord(client_manifest, 'm', getRecordStructSize(server_manifest));
        return;
    }

    /*compute an array of live hashes*/
    char **live_hash_arr = getLiveHashes(client_manifest, client_manifest_size);

    /*create path for commit*/
    char *pclient4 = append_client(10 + strlen(project_name));
    strcat(pclient4, project_name);
    int path_len = strlen("./Commit") + strlen(pclient4);
    char *commit_path = (char *)malloc(path_len + 1 * sizeof(char));
    snprintf(commit_path, path_len + 1, "%s%s", pclient4, "/.Commit");
    commit_path[path_len + 1] = '\0';
    int commit_fd = open(commit_path, O_WRONLY | O_CREAT | O_TRUNC, 00600);

    /*go through each file in client manifest and compare manifests and write commit file*/
    int i = 1;
    while (i < client_manifest_size)
    {
        /*check to see if they modified the code*/
        char *file_name = client_manifest[i]->file;
        Boolean found_file = search_Record(server_manifest, file_name);
        Boolean write_file = false;
        if (found_file == true)
        {
            if (strcmp(live_hash_arr[i], client_manifest[i]->hash) != 0)
            {
                /*check that the client file is up to date*/
                int server_file_version = atoi(server_manifest[i]->version);
                int client_file_version = atoi(client_manifest[i]->version);
                if (server_file_version < client_file_version)
                {
                    free(client_manifest_data);
                    freeRecord(server_manifest, 'm', getRecordStructSize(server_manifest));
                    freeRecord(client_manifest, 'm', getRecordStructSize(server_manifest));
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
        else
        {
            /*write that client added a file*/
            write(commit_fd, "A", 1);
            write_file = true;
        }
        if (write_file == true)
        {
            write(commit_fd, " ", 1);
            write(commit_fd, file_name, strlen(file_name));
            write(commit_fd, " ", 1);
            write(commit_fd, client_manifest[i]->hash, strlen(client_manifest[i]->hash));
            write(commit_fd, "\n", 1);
        }
        i++;
    }
    i = 1;
    while (i < server_manifest_size)
    {
        char *file_name = server_manifest[i]->file;
        Boolean found_file = search_Record(client_manifest, file_name);
        if (found_file == false)
        {
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
    char *commit_file_content = read_file(commit_path);
    char *length_of_commit = to_Str(strlen(commit_file_content));
    char *send_commit_to_server = (char *)malloc(strlen("commit") + digits(strlen(project_name)) + strlen(project_name) + digits(atoi(length_of_commit)) + strlen(commit_file_content) + 4 * sizeof(char));
    send_commit_to_server[0] = '\0';
    strcat(send_commit_to_server, "commit");
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, to_Str(strlen(project_name)));
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, project_name);
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, length_of_commit);
    strcat(send_commit_to_server, ":");
    strcat(send_commit_to_server, commit_file_content);
    char *extended_commit_cmd = (char *)malloc(digits(strlen(send_commit_to_server) + strlen(send_commit_to_server) + 1 * sizeof(char)));
    extended_commit_cmd[0] = '\0';
    strcat(extended_commit_cmd, to_Str(strlen(send_commit_to_server) + 1));
    strcat(extended_commit_cmd, ":");
    block_write(sockfd, extended_commit_cmd, strlen(extended_commit_cmd));

    /*Finally free!*/
    free(commit_path);
    freeRecord(server_manifest, 'm', getRecordStructSize(server_manifest));
    freeRecord(client_manifest, 'm', getRecordStructSize(server_manifest));
    free_string_arr(live_hash_arr, client_manifest_size);
}

//===================================== ADD/REMOVE METHODS ============================================================================
Boolean add_file_to_record(char *projectName, char *fileName, char *recordPath)
{
    int fd = open(recordPath, O_WRONLY | O_APPEND);
    if (fd == -1)
    {
        return false;
    }
    char *manifestData = read_file(recordPath);
    Record **record_arr = create_record_struct(manifestData, true);
    if (search_Record(record_arr, fileName))
    {
        printf("Fatal Error: File %s already exists in Record\n", fileName);
    }
    else
    {
        write(fd, "1", 1);
        write(fd, " ", 1);
        write(fd, fileName, strlen(fileName));
        write(fd, " ", 1);
        char *fileData = read_file(fileName);
        char *hashcode = getHash(fileData);
        write(fd, hashcode, strlen(hashcode));
        write(fd, "\n", 1);
        free(hashcode);
        free(fileData);
    }
    freeRecord(record_arr, 'm', getRecordStructSize(record_arr));
    free(manifestData);
    close(fd);
}

Boolean remove_file_from_record(char *projectName, char *fileName, char *recordPath)
{
    char *fileData = read_file(recordPath);
    Record **record_arr = create_record_struct(fileData, true);
    int x = 1;
    int size = getRecordStructSize(record_arr);
    int fd = open(recordPath, O_WRONLY | O_TRUNC);
    if (fd == -1)
    {
        return false;
    }
    write(fd, record_arr[0]->version, strlen(record_arr[0]->version));
    Boolean remove = false;
    //printf("%s\n", fileName);
    while (x < size)
    {
        if (strcmp(fileName, record_arr[x]->file) != 0)
        {
            char *temp = printRecord(record_arr[x]);
            write(fd, temp, strlen(temp));
            write(fd, "\n", 1);
            //printf("%s\n", temp);
            free(temp);
        }
        else
        {
            remove = true;
        }
        x++;
    }
    if (!remove)
    {
        printf("Fatal Error: record does not contain file\n");
        return false;
    }
    freeRecord(record_arr, 'm', getRecordStructSize(record_arr));
    free(fileData);
    close(fd);
}

int read_len_message(int fd)
{
    //printf("um");
    char *buffer = (char *)malloc(sizeof(char) * 50);
    bzero(buffer, 50);
    int status = 0;
    int readIn = 0;
    do
    {
        status = read(fd, buffer + readIn, 1);
        readIn += status;
        //printf("hey");
    } while (buffer[readIn - 1] != ':' && status > 0);
    char *num = (char *)malloc(sizeof(char) * strlen(buffer));
    strncpy(num, buffer, strlen(buffer) - 1);
    num[strlen(buffer) - 1] = '\0';
    free(buffer);
    int len = atoi(num); //- strlen(num) - 1;
    //printf("%d\n", len);
    free(num);
    return len;
}
/* blocking read */
char *block_read(int fd, int targetBytes)
{
    char *buffer = (char *)malloc(sizeof(char) * targetBytes + 1);
    memset(buffer, '\0', targetBytes + 1);
    int status = 0;
    int readIn = 0;
    do
    {
        status = read(fd, buffer + readIn, targetBytes - readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    buffer[targetBytes] = '\0';
    if (readIn < 0)
        printf("ERROR reading from socket");
    return buffer;
}
//=========================== UPGRADE METHODS==================================================================
char *fetch_file_from_server(char *fileName, int sockfd)
{
    int filePathLen = strlen(fileName);
    int digLenFilePath = digits(filePathLen);
    int lenMessage = filePathLen + digLenFilePath + 2 + strlen("sendfile");
    int totalLen = digits(lenMessage) + lenMessage + 1;
    char *command = (char *)malloc(sizeof(char) * totalLen + 1);
    command[0] = '\0';
    char *digLenFilePathChar = to_Str(digLenFilePath);
    char *messageChar = to_Str(lenMessage);
    strcpy(command, messageChar);
    strcat(command, ":sendfile:");
    strcat(command, digLenFilePathChar);
    strcat(command, ":");
    strcat(command, fileName);
    printf("%s\n", command);
    block_write(sockfd, command, totalLen);
    int messageLen = read_len_message(sockfd);
    printf("Receieved from Server a message of length: %d\n", messageLen);
    char *serverData = block_read(sockfd, messageLen);
    if (strstr(serverData, "ERROR") != NULL) // check if errored (project name does not exist on server)
    {
        printf("%s", serverData);
        return NULL;
    }
    printf("%s\n", serverData);
    free(digLenFilePathChar);
    free(messageChar);
    free(command);
    return serverData;
}
void write_record_to_file(int fd, Record **records, Boolean append, int size)
{
    int s = size;
    int x = 0;
    if (!append)
    {
        x = 1;
        write(fd, records[0]->version, strlen(records[0]->version));
    }
    while (x < s)
    {
        if (records[x] != NULL)
        {
            char *temp = printRecord(records[x]);
            write(fd, temp, strlen(temp));
            write(fd, "\n", 1);
            free(temp);
        }
        x++;
    }
}
void add_file_to_manifest(char *hashS, char *versionS, char *fileS, Record **manifest, int count)
{
    Record *record = (Record *)malloc(sizeof(Record));
    /* set hash */
    char *hash = (char *)malloc(sizeof(char) * strlen(hashS) + 1);
    strcpy(hash, hashS);
    record->hash = hash;
    /* set file */
    char *file = (char *)malloc(sizeof(char) * strlen(fileS) + 1);
    strcpy(file, fileS);
    record->file = file;
    /* set version */
    char *version = (char *)malloc(sizeof(char) * strlen(versionS) + 1);
    strcpy(version, versionS);
    record->version = version;
    /* set record */
    manifest[count] = record;
}
int find_number_of_adds(Record **records)
{
    int x = 1;
    int count = 0;
    int size = getRecordStructSize(records);
    while (x < size)
    {
        if (strcmp(records[x]->version, "'A") == 0)
        {
            count++;
        }
        x++;
    }
    return count;
}
void upgrade(char *projectName, int sockfd)
{
    // check if there is  .Update file or .Conflict file
    char *updateFilePath = (char *)malloc(strlen(projectName) + 1 + 8);
    strcpy(updateFilePath, projectName);
    strcat(updateFilePath, "/.Update");
    char *conflictFilePath = (char *)malloc(strlen(projectName) + 1 + 10);
    strcpy(conflictFupilePath, projectName);
    strcat(conflictFilePath, "/.Conflict");
    char *tempServerFilePath = (char *)malloc(strlen(projectName) + 1 + 16);
    strcpy(tempServerFilePath, projectName);
    strcat(tempServerFilePath, "/.ServerManifest");
    char *manifestFilePath = (char *)malloc(strlen(projectName) + 1 + 10);
    strcpy(manifestFilePath, projectName);
    strcat(manifestFilePath, "/.Manifest");
    Boolean u = fileExists(updateFilePath);
    Boolean c = fileExists(conflictFilePath);
    if (!u && !c)
    {
        // fix print statements
        free(updateFilePath);
        free(conflictFilePath);
        return;
    }
    if (c)
    {
        free(updateFilePath);
        free(conflictFilePath);
        printf("Please resolve all conflicts and update before upgrading.\n");
        return;
    }
    // check if .update is empty
    if (u)
    {
        char *content = read_file(updateFilePath);
        char *serverContent = read_file(tempServerFilePath);
        char *fileData = read_file(manifestFilePath);
        if (content == NULL || strcmp(content, "") == 0)
        {
            printf("Project is up to date");
            unlink(updateFilePath);
            free(updateFilePath);
            free(conflictFilePath);
            return;
        }
        //printf("%s\n", content);
        Record **updateRecords = create_record_struct(content, false);
        Record **serverRecords = create_record_struct(serverContent, true);
        Record **manifestClient = create_record_struct(fileData, true);
        int adds = find_number_of_adds(updateRecords);
        printf("Number of Adds %d\n", adds);
        Record **addRecords = (Record **)malloc(sizeof(Record *) * adds);
        int countAdd = 0;
        //printAllRecords(updateRecords);
        int size = getRecordStructSize(updateRecords);
        int x = 1;
        // proccess updates
        while (x < size)
        {
            if (strcmp(updateRecords[x]->version, "'D") == 0)
            {
                int index = search_record_hash(manifestClient, updateRecords[x]->file);
                if (index != -1)
                {
                    free(manifestClient[index]->hash);
                    free(manifestClient[index]->version);
                    free(manifestClient[index]->file);
                    free(manifestClient[index]);
                    manifestClient[index] = NULL;
                }
            }
            else if (strcmp(updateRecords[x]->version, "'A") == 0)
            {
                // fetch server file
                char *newContent = fetch_file_from_server(updateRecords[x]->file, sockfd);
                if (newContent == NULL)
                {
                    return;
                }
                // write content to file
                int fd = open(updateRecords[x]->file, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if (fd == -1)
                {
                    printf("Fatal Error: Could not open Update file");
                }
                block_write(fd, newContent, strlen(newContent));
                // add to manifest
                int index = search_record_hash(serverRecords, updateRecords[x]->file);
                free(newContent);
                // printf("INDEX: %d", index);
                add_file_to_manifest(serverRecords[index]->hash, serverRecords[index]->version, serverRecords[index]->file, addRecords, countAdd);
                countAdd++;
            }
            else if (strcmp(updateRecords[x]->version, "'M") == 0)
            {
                // fetch server file
                char *newContent = fetch_file_from_server(updateRecords[x]->file, sockfd);
                if (newContent == NULL)
                {
                    return;
                }
                // write content to file
                int fd = open(updateRecords[x]->file, O_WRONLY | O_TRUNC);
                if (fd == -1)
                {
                    printf("Fatal Error: Could not open Update file");
                }
                block_write(fd, newContent, strlen(newContent));
                // modify file to manifest
                int indexS = search_record_hash(serverRecords, updateRecords[x]->file);
                int indexC = search_record_hash(manifestClient, updateRecords[x]->file);
                free(manifestClient[indexC]->hash);
                free(manifestClient[indexC]->version);
                /* set hash */
                char *hash = (char *)malloc(sizeof(char) * strlen(serverRecords[indexS]->hash) + 1);
                strcpy(hash, serverRecords[indexS]->hash);
                /* set version */
                char *version = (char *)malloc(sizeof(char) * strlen(serverRecords[indexS]->version) + 1);
                strcpy(version, serverRecords[indexS]->version);
                manifestClient[indexC]->hash = hash;
                manifestClient[indexC]->version = version;
                free(newContent);
                close(fd);
            }
            x++;
        }
        printf("Add Count %d\n", countAdd);
        //printAllRecords(manifestClient);
        int fd = open(manifestFilePath, O_WRONLY | O_TRUNC);
        if (fd == -1)
        {
            printf("Fatal Error: Could not open Update file");
        }
        write_record_to_file(fd, manifestClient, false, getRecordStructSize(manifestClient));
        close(fd);
        if (countAdd > 0)
        {
            fd = open(manifestFilePath, O_WRONLY | O_APPEND);
            if (fd == -1)
            {
                printf("Fatal Error: Could not open Update file");
            }
            write_record_to_file(fd, addRecords, true, countAdd);
            close(fd);
            freeRecord(addRecords, 'a', countAdd);
        }
        else
        {
            free(addRecords);
        }
        // delete .update file
        unlink(updateFilePath);
        unlink(tempServerFilePath);
        free(updateFilePath);
        free(conflictFilePath);
        printAllRecords(updateRecords);
        //freeRecord(updateRecords, 'u', getRecordStructSize(updateRecords));
        freeRecord(manifestClient, 'm', getRecordStructSize(manifestClient));
        freeRecord(serverRecords, 'm', getRecordStructSize(serverRecords));
    }
}

//=========================== UPDATE METHODS==================================================================
int read_len_message(int fd)
{
    //printf("um");
    char *buffer = (char *)malloc(sizeof(char) * 50);
    bzero(buffer, 50);
    int status = 0;
    int readIn = 0;
    do
    {
        status = read(fd, buffer + readIn, 1);
        readIn += status;
        //printf("hey");
    } while (buffer[readIn - 1] != ':' && status > 0);
    char *num = (char *)malloc(sizeof(char) * strlen(buffer));
    strncpy(num, buffer, strlen(buffer) - 1);
    num[strlen(buffer) - 1] = '\0';
    free(buffer);
    int len = atoi(num); //- strlen(num) - 1;
    //printf("%d\n", len);
    free(num);
    return len;
}
/* blocking read */
char *block_read(int fd, int targetBytes)
{
    char *buffer = (char *)malloc(sizeof(char) * targetBytes + 1);
    memset(buffer, '\0', targetBytes + 1);
    int status = 0;
    int readIn = 0;
    do
    {
        status = read(fd, buffer + readIn, targetBytes - readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    buffer[targetBytes] = '\0';
    if (readIn < 0)
        printf("ERROR reading from socket");
    return buffer;
}

void update(char *projectName, int sockfd)
{
    // create client manifest struct
    if (!searchForProject(projectName))
    {
        printf("Warning: Project name does not exist on client side. Please run checkout\n");
        return;
    }
    char *manifestFilePath = (char *)malloc(strlen(projectName) + 1 + 10);
    strcpy(manifestFilePath, projectName);
    strcat(manifestFilePath, "/.Manifest");
    char *fileData = read_file(manifestFilePath);
    Record **manifestClient = create_record_struct(fileData, true);
    // free(manifestFilePath);
    free(fileData);
    /* send manifest command */
    int projNumLen = digits(strlen(projectName));
    int lenMessage = strlen(projectName) + strlen("manifest") + projNumLen + 2;  // len of message
    int totalLen = digits(lenMessage) + lenMessage + 1;                          // digit length + lenMessage + colon
    char *serverManifestCommand = (char *)malloc(sizeof(char) * (totalLen + 1)); // total len + null terminator
    check_malloc_null(serverManifestCommand);
    char *projLenChar = to_Str(strlen(projectName));
    char *totalLenChar = to_Str(lenMessage);
    serverManifestCommand[0] = '\0';
    strcpy(serverManifestCommand, totalLenChar);
    strcat(serverManifestCommand, ":manifest:");
    strcat(serverManifestCommand, projLenChar);
    strcat(serverManifestCommand, ":");
    strcat(serverManifestCommand, projectName);
    printf("Sending to Server: %s\n", serverManifestCommand);
    write(sockfd, serverManifestCommand, totalLen);
    free(serverManifestCommand);
    free(projLenChar);
    free(totalLenChar);
    /* recieve manifest data */
    int messageLen = read_len_message(sockfd);
    printf("Receieved from Server a message of length: %d\n", messageLen);
    /* create server manifest struct */
    char *serverData = block_read(sockfd, messageLen);
    if (strstr(serverData, "ERROR") != NULL) // check if errored (project name does not exist on server)
    {
        printf("%s", serverData);
        return;
    }
    Record **manifestServer = create_record_struct(serverData, true);
    free(serverData);
    // printAllRecords(manifestClient);
    /*check versions of both structs*/
    // versions are the same
    /* Update */
    char *updateFilePath = (char *)malloc(strlen(projectName) + 1 + 8);
    strcpy(updateFilePath, projectName);
    strcat(updateFilePath, "/.Update");
    int ufd = open(updateFilePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    if (ufd == -1)
    {
        printf("Fatal Error: Could not open Update file");
    }
    /* Conflict */
    char *conflictFilePath = (char *)malloc(strlen(projectName) + 1 + 10);
    strcpy(conflictFilePath, projectName);
    strcat(conflictFilePath, "/.Conflict");
    int cfd = open(conflictFilePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    if (cfd == -1)
    {
        printf("Fatal Error: Could not open Update file");
    }
    /* Temp */
    char *tempFilePath = (char *)malloc(strlen(projectName) + 1 + 6);
    strcpy(tempFilePath, projectName);
    strcat(tempFilePath, "/.Temp");
    int fd = open(tempFilePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    if (fd == -1)
    {
        printf("Fatal Error: Could not open file");
    }
    /* Temp Server Path */
    char *tempServerFilePath = (char *)malloc(strlen(projectName) + 1 + 16);
    strcpy(tempServerFilePath, projectName);
    strcat(tempServerFilePath, "/.ServerManifest");
    int tfd = open(tempServerFilePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    if (tfd == -1)
    {
        printf("Fatal Error: Could not open file");
    }
    if (strcmp(manifestServer[0]->version, manifestClient[0]->version) == 0)
    {
        printf("No updates from server\n");
        close(ufd);
        close(cfd);
        close(fd);
        unlink(conflictFilePath);
        unlink(tempFilePath);
        free(tempFilePath);
        free(updateFilePath);
        free(conflictFilePath);
        freeRecord(manifestClient, 'm', getRecordStructSize(manifestClient));
        freeRecord(manifestServer, 'm', getRecordStructSize(manifestServer));
        return;
    }
    int size = getRecordStructSize(manifestServer);
    int x = 1;
    Boolean conflict = false;
    // for each file in the server manifest return hash or NULL
    while (x < size)
    {
        int clientIndex = search_record_hash(manifestClient, manifestServer[x]->file);
        int len = 4 + strlen(manifestServer[x]->file) + strlen(manifestServer[x]->hash);
        char *temp = (char *)malloc(sizeof(char *) * (len) + 1);
        if (clientIndex == -1)
        {
            printf("'A %s\n", manifestServer[x]->file);
            snprintf(temp, len + 1, "%s %s %s", "'A", manifestServer[x]->file, manifestServer[x]->hash);
            block_write(fd, temp, len);
            write(fd, "\n", 1);
            /* will remove this if I decide to add to upgrade */
        }
        else
        {
            char *clientHash = manifestClient[clientIndex]->hash;
            if (strcmp(manifestServer[x]->version, manifestClient[clientIndex]->version) != 0)
            {
                char *currentFileData = read_file(manifestServer[x]->file);
                char *currentHash = getHash(currentFileData);
                if (strcmp(currentHash, clientHash) == 0 && strcmp(manifestServer[x]->hash, clientHash) != 0)
                { // live hash and manifest client hash same
                    printf("'M %s\n", manifestServer[x]->file);
                    snprintf(temp, len + 1, "%s %s %s", "'M", manifestServer[x]->file, manifestServer[x]->hash);
                    block_write(fd, temp, len);
                    write(fd, "\n", 1);
                }
                else if (strcmp(currentHash, clientHash) != 0 && strcmp(manifestServer[x]->hash, clientHash) != 0)
                {
                    printf("'C %s\n", manifestServer[x]->file);
                    snprintf(temp, len + 1, "%s %s %s", "'C", manifestServer[x]->file, manifestServer[x]->hash);
                    block_write(fd, temp, len);
                    write(fd, "\n", 1);
                    conflict = true;
                }
                free(currentFileData);
                free(currentHash);
            }
            free(currentFileData);
            free(currentHash);
        }
        free(temp);
        x++;
    }
    size = getRecordStructSize(manifestClient);
    x = 1;
    while (x < size)
    {
        int serverIndex = search_record_hash(manifestServer, manifestClient[x]->file);
        if (serverIndex == -1)
        { // file from client is not in server.
            //char *serverHash = manifestServer[serverIndex]->hash;
            int len = 4 + strlen(manifestClient[x]->file) + strlen(manifestClient[x]->hash);
            char *temp = (char *)malloc(sizeof(char *) * (len) + 1);
            printf("'D %s\n", manifestClient[x]->file);
            snprintf(temp, len + 1, "%s %s %s", "'D", manifestClient[x]->file, manifestClient[x]->hash);
            block_write(fd, temp, len);
            write(fd, "\n", 1);
            free(temp);
        }
        x++;
    }
    close(fd);
    fd = open(tempFilePath, O_RDONLY);
    if (fd == -1)
    {
        printf("Fatal Error: Could not open file");
    }
    if (conflict == true)
    {
        char *fileData = read_file(tempFilePath);
        write(cfd, fileData, strlen(fileData));
        free(fileData);
        close(ufd);
        close(cfd);
        unlink(updateFilePath);
        write_record_to_file(tfd, manifestServer, false, getRecordStructSize(manifestServer));
    }
    else
    {
        char *fileData = read_file(tempFilePath);
        write(ufd, fileData, strlen(fileData));
        free(fileData);
        close(ufd);
        close(cfd);
        unlink(conflictFilePath);
        write_record_to_file(tfd, manifestServer, false, getRecordStructSize(manifestServer));
    }
    freeRecord(manifestClient, 'm', getRecordStructSize(manifestClient));
    freeRecord(manifestServer, 'm', getRecordStructSize(manifestServer));
    unlink(tempFilePath);
    free(tempFilePath);
    free(updateFilePath);
    free(tempServerFilePath);
    free(conflictFilePath);
    free(manifestFilePath);
}

//=================================================== CREATE ===============================================================
/*Creates the project in the client folder*/
void parseBuffer_create(char *buffer)
{
    int bcount = 0;
    int toklen = -1;
    char *tok = NULL;
    char *write_to_file = NULL;
    int first_num_len = getUnknownLen(bcount, buffer);
    tok = getSubstring(bcount, buffer, first_num_len);
    bcount += strlen(tok);
    bcount++;
    while (bcount < strlen(buffer))
    {
        if (toklen < 0)
        {
            toklen = getUnknownLen(bcount, buffer);
        }
        tok = getSubstring(bcount, buffer, toklen);
        toklen = -1;
        if (isdigit(tok[0]))
        { //token is a number
            toklen = atoi(tok);
        }
        else
        {
            /*create project*/
            if (tok[0] == 'P')
            {
                char *projectName = substr(tok, 1, strlen(tok));
                mkdir_recursive(projectName);
                int ch = chmod("./client", 0775);
                if (ch < 0)
                    printf("ERROR setting perrmissions.\n");
                ch = chmod(projectName, 0775);
                if (ch < 0)
                    printf("ERROR setting perrmissions: %s.\n", strerror(errno));
            }
            /*create file*/
            else if (tok[0] == 'F')
            {
                char *filePath = substr(tok, 1, strlen(tok));
                write_to_file = filePath;
                int n = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if (n < 0)
                    printf("ERROR making file.\n");
            }
            /*write datainto file */
            else if (tok[0] == 'C')
            {
                char *fileContent = substr(tok, 1, strlen(tok));
                int fn = open(write_to_file, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if (fn < 0)
                    printf("ERROR could not open file to write to.\n");
                int n = write(fn, fileContent, strlen(fileContent));
                if (n < 0)
                    printf("ERROR writing data.\n");
            }
        }
        bcount += strlen(tok);
        bcount++; //this is for the semicolon
    }
    free(tok);
}

//=========================== SOCKET/CONFIGURE METHODS==================================================================
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

/* blocking write */
void block_write(int fd, char *data, int targetBytes)
{
    int status = 0;
    int readIn = 0;
    do
    {
        status = write(fd, data + readIn, targetBytes - readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    if (readIn < 0)
        printf("ERROR writing to socket");
}

/* Connect to Server*/
int create_socket(char *host, char *port)
{
    int portno = atoi(port);
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // create socket and format hostname
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(host);
    if (server == NULL)
    {
        printf("ERROR, no such host\n");
        exit(0);
    }

    /* create struct*/
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Tries to connect to server every 3 seconds until successful */
    int status = 0;
    int count = 0;
    do
    {
        status = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (status < 0)
        {
            printf("Connection Attempt #%d\n", count);
        }
        delay(10);
        count++;
    } while (status < 0);

    /* Exchange initial connection messages*/
    block_write(sockfd, "48:Incoming client connection connection successful", 51);
    int len = read_len_message(sockfd);
    //printf("%d\n", len);
    char *buffer = block_read(sockfd, len);
    printf("%s\n", buffer);
    free(buffer);

    return sockfd;
}

/* Create Configure File */
void write_configure(char *hostname, char *port)
{
    int outputFile = open("./.configure", O_WRONLY | O_CREAT | O_TRUNC, 00600); // creates a file if it does not already exist
    if (outputFile == -1)
    {
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
int read_configure_and_connect()
{
    int sockfd;
    char *fileData = read_file("./.configure");
    if (fileData == NULL)
    {
        printf("Fatal Error: Configure File not found.\n");
        return;
    }
    char *host = strtok(fileData, "\n");
    char *port = strtok(NULL, "\n");
    if (port != NULL && host != NULL)
    {
        sockfd = create_socket(host, port);
    }
    free(fileData);
    return sockfd;
}

/*Sending command to create project in server. Returns the fd*/
int write_to_server(int sockfd, char *argv1, char *argv2)
{
    int clen = strlen(argv2) + digits(strlen(argv2)) + strlen(argv1) + 2;
    char *command = (char *)malloc(clen + 1 * sizeof(char));
    snprintf(command, clen + 1, "%s:%d:%s", argv1, strlen(argv2), argv2);
    char *new_command = (char *)malloc(strlen(command) + digits(strlen(command)) + 1 * sizeof(char));
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
char *read_from_server(int sockfd)
{
    /*Read the project name sent from the server.*/
    int len = read_len_message(sockfd);
    char *buffer = block_read(sockfd, len);
    printf("%s\n", buffer);
    return buffer;
}

// MAIN METHOD  ================================================================================
int main(int argc, char **argv)
{
    int sockfd;
    if (argc == 4 && strcmp(argv[1], "configure") == 0)
    {
        write_configure(argv[2], argv[3]);
    }
    else if (argc == 3 && (strcmp(argv[1], "create") == 0 || strcmp(argv[1], "checkout") == 0))
    {
        sockfd = read_configure_and_connect();
        int n = write_to_server(sockfd, argv[1], argv[2]);
        if (n < 0)
            printf("ERROR writing to socket.\n");
        else
        {
            char *buffer = read_from_server(sockfd);
            parseBuffer_create(buffer);
        }
        /*disconnect server at the end!*/
        block_write(sockfd, "6:Done", 6);
        printf("Client Disconnecting");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "destroy") == 0))
    {
        sockfd = read_configure_and_connect();
        int n = write_to_server(sockfd, argv[1], argv[2]);
        if (n < 0)
            printf("ERROR writing to socket.\n");
        else
        {
            char *buffer = read_from_server(sockfd);
        }
        /*disconnect server at the end!*/
        block_write(sockfd, "6:Done", 6);
        printf("Client Disconnecting");
        close(sockfd);
    }
    else if (argc == 4 && (strcmp(argv[1], "add") == 0 || strcmp(argv[1], "remove") == 0))
    {
        // combine project name and file path
        char *filePath = (char *)malloc(strlen(argv[2]) + strlen(argv[3]) + 2);
        check_malloc_null(filePath);
        strcpy(filePath, argv[2]);
        strcat(filePath, "/");
        strcat(filePath, argv[3]);

        if (searchForProject(argv[2]))
        {
            /* create manifest file path */
            char *manifestFilePath = (char *)malloc(strlen(argv[2]) + 1 + 10);
            strcpy(manifestFilePath, argv[2]);
            strcat(manifestFilePath, "/.Manifest");

            /* if manifest exists */
            if (fileExists(manifestFilePath))
            {
                if (fileExists(filePath) && (strcmp(argv[1], "add") == 0))
                {
                    add_file_to_record(argv[2], filePath, manifestFilePath);
                }
                else if (strcmp(argv[1], "remove") == 0)
                {
                    remove_file_from_record(argv[2], filePath, manifestFilePath);
                }
            }
            free(manifestFilePath);
        }
        free(filePath);
    }
    else if (argc == 3 && (strcmp(argv[1], "update") == 0))
    {
        /* Need to handle update error : project does not exist on server */
        sockfd = read_configure_and_connect();
        update(argv[2], sockfd);
        block_write(sockfd, "6:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "upgrade") == 0))
    {
        /* Need to handle update error : project does not exist on server */
        sockfd = read_configure_and_connect();
        upgrade(argv[2], sockfd);
        block_write(sockfd, "6:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "commit") == 0))
    {
        //pretty sure commit does not work right now? idk didn't test yet, im scared

        // sockfd = read_configure_and_connect();
        // int n = write_to_server(sockfd, "manifest", argv[2]);
        // if(n < 0)
        //     printf("ERROR writing to socket.\n");
        // else{
        //     char* buffer = read_from_server(sockfd);
        //     write_commit_file(sockfd, argv[2], buffer);
        // }
    }
    else if (argc == 3 && (strcmp(argv[1], "push") == 0))
    {
    }
    else
    {
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
