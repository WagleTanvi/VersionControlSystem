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

void untarFile(char *file)
{
    char *command = malloc(sizeof(char) + 1 + 8 + strlen(file));
    strcpy(command, "tar -xf ");
    // strcat(command, "./history/");
    strcat(command, file);
    //strcat(command, " -C .");
    printf("%s\n", command);
    system(command);
}

char *extract_path(char *path)
{
    int size = strlen(path);
    int x = size - 1;
    while (x >= 0)
    {
        if (path[x] == '/')
        {
            break;
        }
        x--;
    }
    char *temp = (char *)malloc(sizeof(char) * x + 2);
    strncpy(temp, path, x + 1);
    temp[x + 1] = '\0';
    //printf("[SERVER] %s\n", temp);
    return temp;
}

Boolean search_tars(char *tar)
{
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir("./history");
    if (dir == NULL)
    {
        return false;
    }
    while ((d = readdir(dir)) != NULL)
    {
        if (strcmp(d->d_name, tar) == 0)
        {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

/*Returns all the projects in the CWD*/
int find_all_projects(){
    char path[4096];
    int count = 0;
    struct dirent *d;
    DIR *dir = opendir("./");
    if (dir == NULL){
        return 0;
    }
    while ((d = readdir(dir)) != NULL) {
        if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, "header-files") == 0) continue;
        if (d->d_type == DT_DIR){
            count++;
        }
    }
    closedir(dir);
    return count;
}

/*Returns the names of all the  projects in the CWD as a string array*/
char** get_project_names(int size){
    char path[4096];
    char** project_names = (char**)malloc(size*sizeof(char*));
    struct dirent *d;
    DIR *dir = opendir("./");
    if (dir == NULL){
        return NULL;
    }
    int i = 0;
    while ((d = readdir(dir)) != NULL) {
        if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, "header-files") == 0) continue;
        if (d->d_type == DT_DIR){
            project_names[i] = d->d_name;
            i++;
        }
    }
    closedir(dir);
    return project_names;
}

/*Returns a string to send to current version*/
char *current_version_format(char *req_dir)
{
    char *len = to_Str(strlen(req_dir));
    char *formatted = (char *)malloc(strlen("currentversion") + 1 + digits(strlen(req_dir)) + 1 + strlen(req_dir));
    formatted[0] = '\0';
    strcat(formatted, "currentversion:");
    strcat(formatted, len);
    strcat(formatted, ":");
    strcat(formatted, req_dir);

    free(len);
    return formatted;
}

int get_version_from_tar(char* project_name, char* tar_name){
    int count = 0;
    count += strlen(project_name);
    count++;
    char version[5];
    int i = 0;
    while(tar_name[count] != '.'){
        version[i] = tar_name[count];
        count++;
        i++;
    }
    int version_num = atoi(version);
    return version_num;
}

void remove_new_versions(char* project_name, int req_version){
    /*remove the files in pending commits*/
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir("history");
    if (dir == NULL)
    {
        printf("ERROR not a directory.\n");
        return;
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", "history", d->d_name);
        if (d->d_type != DT_DIR)
        {
            int tar_version = get_version_from_tar(project_name, d->d_name);
            if(tar_version >= req_version){
                unlink(path);
            }
        }
    }
}

/*Given a project name, duplicate the directory*/
void duplicate_dir(char *project_path, const char *new_project_path, int history, int version)
{
    if(history > version){
        return;
    }
    char path[4096];
    char newpath[4096];
    struct dirent *d;
    DIR *dir = opendir(project_path);
    if (dir == NULL)
    {
        printf("[SERVER] ERROR this is not a directory.\n");
        return;
    }
    mkdir_recursive(new_project_path);
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", project_path, d->d_name);
        snprintf(newpath, 4096, "%s/%s", new_project_path, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0 || strcmp(d->d_name, "pending-commits") == 0)
            continue;
        else if(strcmp(d->d_name, "history")==0){
            history ++;
        }
        if (d->d_type == DT_DIR)
        {
            duplicate_dir(path, newpath, history, version);
        }
        else
        {
            int dup_file = open(newpath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
            if (dup_file < 0)
            {
                printf("[SERVER] ERROR unable to make new file: %s\n", strerror(errno));
            }
            char *old_file_contents = getFileContent(path, "");
            write(dup_file, old_file_contents, strlen(old_file_contents));
        }
    }
    closedir(dir);
}