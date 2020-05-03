#include "helper.h"
#include "record.h"

//================== HELPER METHODS========================================
/*Count digits in a number*/
int digits(int n)
{
    int count = 0;
    while (n != 0)
    {
        n /= 10; // n = n/10
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

/*Get substring of a string*/
char *getSubstring(int bcount, char *buffer, int nlen)
{
    char *substr = (char *)malloc(nlen + 1 * sizeof(char));
    check_malloc_null(substr);
    int count = 0;
    while (count < nlen)
    {
        substr[count] = buffer[bcount];
        count++;
        bcount++;
    }
    substr[nlen] = '\0';
    return substr;
}

/*Makes directory and all subdirectories*/
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

/*Return string of file content*/
char *getFileContent(char *file, char *flag)
{
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        printf("ERROR opening file: %s\n", strerror(errno));
        return NULL;
    }
    struct stat stats;
    if (stat(file, &stats) == 0)
    {
        int fileSize = stats.st_size;
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

        /*Append the necessary flag!*/
        if (strcmp(flag, "") == 0)
            return buffer;
        else
        {
            char f[2] = "C\0";
            char *new_buffer = (char *)malloc(strlen(buffer) + 2 * sizeof(char));
            check_malloc_null(new_buffer);
            int i = 0;
            while (i < 2)
            {
                new_buffer[i] = f[i];
                i++;
            }
            strcat(new_buffer, buffer);
            free(buffer);
            return new_buffer;
        }
    }
    close(fd);
    printf("Warning: stat error. \n");
    return NULL;
}

/*Returns if server has the given project - needs the full path.*/
Boolean search_proj_exists(char *project_name)
{
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir("./");
    if (dir == NULL)
    {
        return false;
    }
    while ((d = readdir(dir)) != NULL)
    {
        if (strcmp(d->d_name, project_name) == 0)
        {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

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

// // Returns hostname for the local computer
// void checkHostName(int hostname)
// {
//     if (hostname == -1)
//     {
//         perror("gethostname");
//     }
// }

// // Returns host information corresponding to host name
// void checkHostEntry(struct hostent * hostentry)
// {
//     if (hostentry == NULL)
//     {
//         perror("gethostbyname");
//     }
// }

// // Converts space-delimited IPv4 addresses
// // to dotted-decimal format
// void checkIPbuffer(char *IPbuffer)
// {
//     if (NULL == IPbuffer)
//     {
//         perror("inet_ntoa");
//     }
// }

// /*Returns the IP address of a machine!*/
// char* get_host_name()
// {
//     char hostbuffer[256];
//     char *IPbuffer;
//     struct hostent *host_entry;
//     int hostname;

//     // To retrieve hostname
//     hostname = gethostname(hostbuffer, sizeof(hostbuffer));
//     checkHostName(hostname);

//     // To retrieve host information
//     host_entry = gethostbyname(hostbuffer);
//     checkHostEntry(host_entry);

//     // To convert an Internet network address into ASCII string
//     IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));

//     return IPbuffer;
// }

char *fetch_file_from_client(char *fileName, int clientSoc)
{
    char *cmd = (char *)malloc(strlen("sendfile") + 1 + digits(strlen(fileName)) + 1 + strlen(fileName) * sizeof(char));
    cmd[0] = '\0';
    strcat(cmd, "sendfile");
    strcat(cmd, ":");
    strcat(cmd, to_Str(strlen(fileName)));
    strcat(cmd, ":");
    strcat(cmd, fileName);
    char *ext_cmd = (char *)malloc(digits(strlen(cmd)) + 1 * sizeof(char));
    ext_cmd[0] = '\0';
    strcat(ext_cmd, to_Str(strlen(cmd)));
    strcat(ext_cmd, ":");
    strcat(ext_cmd, cmd);

    block_write(clientSoc, ext_cmd, strlen(ext_cmd));
    int messageLen = read_len_message(clientSoc);
    char *clientData = block_read(clientSoc, messageLen);
    if (strstr(clientData, "ERROR") != NULL) // check if errored (project name does not exist on server)
    {
        printf("%s", clientData);
        return NULL;
    }
    return clientData;
}

/* tar extra credit */
void tarFile(char *file, char* dir)
{
    char *command = malloc(sizeof(char) + 1 + 21 + (2 * strlen(file)));
    strcpy(command, "tar -cf history/");
    strcat(command, file);
    strcat(command, ".tar ");
    strcat(command, dir);
    printf("%s\n", command);
    system(command);
    //tar -cf history/pojjname.tar proj0
}
// void untarFile(char *file)
// {
//     char *command = malloc(sizeof(char) + 1 + 8 + strlen(file));
//     strcpy(command, "tar -xf ");
//     // strcat(command, "./history/");
//     strcat(command, file);
//     //strcat(command, " -C .");
//     printf("%s\n", command);
//     system(command);
// }