#include "header-files/helper.h"
#include "header-files/record.h"

int cmd_count = 0;
  
// Returns hostname for the local computer 
void checkHostName(int hostname) 
{ 
    if (hostname == -1) 
    { 
        perror("gethostname"); 
    } 
}

// Returns host information corresponding to host name 
void checkHostEntry(struct hostent * hostentry) 
{ 
    if (hostentry == NULL) 
    { 
        perror("gethostbyname"); 
    } 
} 
  
// Converts space-delimited IPv4 addresses 
// to dotted-decimal format 
void checkIPbuffer(char *IPbuffer) 
{ 
    if (NULL == IPbuffer) 
    { 
        perror("inet_ntoa"); 
    } 
} 
  
// Driver code 
char* get_host_name()
{ 
    char hostbuffer[256]; 
    char *IPbuffer; 
    struct hostent *host_entry; 
    int hostname; 
  
    // To retrieve hostname 
    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    checkHostName(hostname); 

    // To retrieve host information 
    host_entry = gethostbyname(hostbuffer); 
    checkHostEntry(host_entry); 
  
    // To convert an Internet network 
    // address into ASCII string 
    IPbuffer = inet_ntoa(*((struct in_addr*) 
                           host_entry->h_addr_list[0])); 
  
    printf("Hostname: %s\n", hostbuffer); 
    printf("Host IP: %s", IPbuffer); 
  
    return IPbuffer; 
} 

char *fetch_file_from_client(char *fileName, int clientSoc)
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
    block_write(clientSoc, command, totalLen);
    int messageLen = read_len_message(clientSoc);
    printf("Receieved from Server a message of length: %d\n", messageLen);
    char *clientData = block_read(clientSoc, messageLen);
    if (strstr(clientData, "ERROR") != NULL) // check if errored (project name does not exist on server)
    {
        printf("%s", clientData);
        return NULL;
    }
    printf("%s\n", clientData);
    free(digLenFilePathChar);
    free(messageChar);
    free(command);
    return clientData;
}

//=============================== HISTORY ======================
/* Not tested */
void save_history(char *commitData, char *projectName, char *version)
{
    char *filePath = (char *)malloc(strlen(projectName) + 1 + 9);
    strcpy(filePath, projectName);
    strcat(filePath, "/.History");
    int fd = open(filePath, O_WRONLY | O_CREAT, 00600);
    if (fd == -1)
    {
        printf("Fatal Error: Could not open History file");
    }
    write(fd, version, strlen(version));
    write(fd, "\n", 1);
    write(fd, commitData, strlen(version));
    free(filePath);
}

//=============================== CURRENT VERSION ======================
/*Returns the current version of a project as a string*/
char *get_current_version(char *buffer, int clientSoc)
{
    /*get project name and check if the project exists in the server*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        free(project_name);
        int n = write(clientSoc, "ERROR project not in the server.\n", 36);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*Read the manifest file*/
    char *manifestpath = (char *)malloc(strlen(project_name) + strlen("./Manifest") * sizeof(char));
    manifestpath[0] = '\0';
    strcat(manifestpath, project_name);
    strcat(manifestpath, "/.Manifest");
    char *manifest_data = getFileContent(manifestpath, "");
    Record **manifest = create_record_struct(manifest_data, 0, 'M');

    return manifest[0]->version;
}

//=============================== ROLLBACK ======================
//23:projectname:13
void rollback(char *buffer, int clientSoc)
{
    /*get project name and check if the project exists in the server*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        free(project_name);
        int n = write(clientSoc, "ERROR project not in the server.\n", 36);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*Get the version number*/
    bcount += (strlen(project_name) + 1);
    int count = bcount;
    while (buffer[count] != '\0')
    {
        count++;
    }
    int nlen = count - bcount;
    int req_version = atoi(getSubstring(bcount, buffer, nlen));

    /*Traverse history folder - if the version number is greater then the requested then delete that version*/
    char *history_dir = (char *)malloc(strlen(project_name) + strlen("/history\0") * sizeof(char));
    history_dir[0] = '\0';
    strcat(history_dir, project_name);
    strcat(history_dir, "/history\0");
    char *new_project_name;
    Boolean found = false;
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(history_dir);
    if (dir == NULL)
    {
        printf("ERROR this is not a directory.\n");
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", history_dir, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type == DT_DIR)
        {
            int version_num = atoi(get_current_version(d->d_name, clientSoc));
            if (version_num == req_version)
            {
                found = true;
                new_project_name = d->d_name;
                break;
            }
            else if (version_num > req_version)
            {
                rmdir(path);
            }
        }
    }
    closedir(dir);
    if (found == false)
    {
        printf("ERROR version was not found. Choose a different version.\n");
        return;
    }

    /*Destory the current proejct and move this project outside the history folder and rename it*/
    char *project_path = (char *)malloc(strlen(project_name) + strlen("/history/") + strlen(new_project_name) * sizeof(char));
    project_path[0] = '\0';
    strcat(project_path, project_name);
    strcat(project_path, "/history\0");
    strcat(project_path, new_project_name);

    char *new_project_path = (char *)malloc(strlen(project_name) + strlen(new_project_name) + 1 * sizeof(char));
    new_project_path[0] = '\0';
    strcat(new_project_path, project_name);
    strcat(new_project_path, "/");
    strcat(new_project_path, new_project_name);

    duplicate_dir(project_path, new_project_path);
    destroyProject(project_name, clientSoc);
    destroyProject(project_path, clientSoc);

    /*Send a success to the client.*/
    block_write(clientSoc, "37:Server has successfully rolled back.\0", 40);
}

//=============================== PUSH ======================
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

/*Write the information in the record to a file*/
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

/*Given a project name, duplicate the directory*/
void duplicate_dir(char *project_path, char *new_project_path)
{
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(project_path);
    if (dir == NULL)
    {
        printf("ERROR this is not a directory.\n");
        return;
    }
    mkdir_recursive(new_project_path);
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", project_path, d->d_name);
        snprintf(path, 4096, "%s/%s", new_project_path, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type == DT_DIR)
        {
            duplicate_dir(path, new_project_path);
        }
        else
        {
            int dup_file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0775);
            if (dup_file < 0)
            {
                printf("ERROR unable to make new file: %s\n", strerror(errno));
            }
        }
    }
    closedir(dir);
}

//push:23:projectname:234:blahblahcommintcontent
void push_commits(char *buffer, int clientSoc)
{
    /*get project name and check if the project exists in the server*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        //free(project_name);
        int n = write(clientSoc, "ERROR project not in the server.\n", 36);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*Get the commit file data*/
    bcount += (strlen(project_name) + 1);
    int count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    int nlen = count - bcount;
    char *size = getSubstring(bcount, buffer, nlen);
    bcount += (strlen(size) + 1);
    char *file_content = getSubstring(bcount, buffer, atoi(size));
    bcount += (strlen(file_content) + 1);

    /*Get server commit file and compare with the client commit file*/
    char* hostname = get_host_name();
    char* pserver = (char*)malloc(strlen(project_name)+strlen(hostname)+strlen("/pending-commits/.Commit-"));
    pserver[0]='\0';
    strcat(pserver, project_name); 
    strcat(pserver, "/pending-commits/.Commit-");
    strcat(pserver, hostname);
    char* server_file_content = getFileContent(pserver, "");
    // if(strcmp(file_content, server_file_content)!=0){
    //     printf("ERROR the client and sever commit files do not match.\n");
    //     return;
    // }
    int num = number_of_lines(server_file_content);

    /*Get the manifest file data*/
    count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    nlen = count - bcount;
    char *size2 = getSubstring(bcount, buffer, nlen);
    bcount += (strlen(size2) + 1);
    char *file_content2 = getSubstring(bcount, buffer, atoi(size2));
    bcount += (strlen(file_content2) + 1);

    /*Get the current server manifest that is in the folder*/
    char *manifestpath = (char *)malloc(strlen(project_name) + strlen("./Manifest") * sizeof(char));
    manifestpath[0] = '\0';
    strcat(manifestpath, project_name);
    strcat(manifestpath, "/.Manifest");
    char *manifest_data = getFileContent(manifestpath, "");
    Record **server_manifest = create_record_struct(manifest_data, num, 'M');
    int server_manifest_size = getRecordStructSize(server_manifest);
    int i = 1;
    while(i < server_manifest_size){
        server_manifest[i] = NULL;
        i++;
    }

    /*Get the client manifest- make it into a record struct*/
    Record **client_manifest = create_record_struct(file_content2, 0, 'M');

    /*make a history folder to store all the old commits*/
    char *history_dir = (char *)malloc(strlen(project_name) + strlen("/history\0") * sizeof(char));
    history_dir[0] = '\0';
    strcat(history_dir, project_name);
    strcat(history_dir, "/history\0");
    mkdir_recursive(history_dir);
    int ch = chmod(history_dir, 0775);
    if (ch < 0)
        printf("ERROR set permission error.\n");

    /*move old project to the history folder*/
    char *new_project_name = (char *)malloc(strlen(project_name) + 3 * sizeof(char));
    int project_version = atoi(server_manifest[0]->version);
    new_project_name[0] = '\0';
    strcat(new_project_name, project_name);
    strcat(new_project_name, "-");
    strcat(new_project_name, to_Str(project_version));

    char *new_project_path = (char *)malloc(strlen(project_name) + strlen(new_project_name) + strlen("history/") * sizeof(char));
    new_project_path[0] = '\0';
    strcat(new_project_path, project_name);
    strcat(new_project_path, "/history/");
    strcat(new_project_path, new_project_name);
    duplicate_dir(project_name, new_project_path);

    /*do the stuff that is in the commit-modify the manifest too*/
    Record **active_commit = create_record_struct(file_content, 0, 'C');
    int commit_size = number_of_lines(file_content);
    int x = 0;
    while (x < commit_size)
    {
        if (strcmp(active_commit[x]->version, "M") == 0)
        { //modify code
            char *filepath = active_commit[x]->file;
            Record *manifest_rec = search_record(server_manifest, filepath);
            Record *client_manifest_rec = search_record(client_manifest, filepath);
            if (manifest_rec != NULL)
            {
                manifest_rec->version = to_Str(atoi(client_manifest_rec->version));
                manifest_rec->hash = active_commit[x]->hash;
            }
            else
            {
                printf("ERROR could not find the file in the manifest. Update?\n");
            }
            char* newContent = fetch_file_from_client(filepath, clientSoc);
            if (newContent == NULL)
            {
                return;
            }
            // write content to file
            int fd = open(filepath, O_WRONLY | O_TRUNC);
            if (fd == -1)
            {
                printf("Fatal Error: Could not open Update file");
            }
            block_write(fd, newContent, strlen(newContent));
        }
        else if (strcmp(active_commit[x]->version, "A") == 0)
        { //add file
            /*make commit file*/
            char *filepath = active_commit[x]->file;
            int new_file = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
            if (new_file < 0)
            {
                printf("ERROR unable to make new file: %s\n", strerror(errno));
                return;
            }
            /*add the new file to the manifest*/
            Record *manifest_rec = search_record(server_manifest, filepath);
            if (manifest_rec != NULL)
            {
                printf("ERROR already exists in manifest.\n");
            }
            else
            {
                Record *new_manifest_rec = (Record *)malloc(sizeof(Record));
                new_manifest_rec->version = "1";
                new_manifest_rec->file = filepath;
                new_manifest_rec->hash = active_commit[x]->hash;
                int m = 0;
                while (server_manifest[m] != NULL && m < server_manifest_size)
                {
                    m++;
                }
                if(server_manifest[m]==NULL && m != server_manifest_size){
                    server_manifest[m] = new_manifest_rec;
                } 
            }
            char *newContent = fetch_file_from_client(filepath, clientSoc);
            if (newContent == NULL)
            {
                return;
            }
            // write content to file
            int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
            if (fd == -1)
            {
                printf("Fatal Error: Could not open Update file");
            }
            block_write(fd, newContent, strlen(newContent));
        }
        else if (strcmp(active_commit[x]->version, "D") == 0)
        { //delete file
            char *filepath = active_commit[x]->file;
            int r = unlink(filepath);
            Record *new_manifest_rec = search_record(server_manifest, filepath);
            if (new_manifest_rec != NULL)
            {
                new_manifest_rec = NULL;
            }
            else
            {
                printf("ERROR could not find the file in the manifest. Update?\n");
            }
        }
        else
        {
            printf("ERROR action not implemented.\n");
        }
        x++;
    }

    /*re-make the manifest file*/
    int u = remove(manifestpath);
    int new_manifest_file = open(manifestpath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
    x = 0;
    while (x < server_manifest_size)
    {
        if (x == 0)
        {
            char *new_vers = to_Str(atoi(server_manifest[0]->version) + 1);
            write(new_manifest_file, new_vers, strlen(new_vers));
            write(new_manifest_file, "\n", 1);
        }
        else
        {
            if (server_manifest[x] == NULL)
            {
                x++;
                continue;
            }
            write(new_manifest_file, server_manifest[x]->version, strlen(server_manifest[x]->version));
            write(new_manifest_file, " ", 1);
            write(new_manifest_file, server_manifest[x]->file, strlen(server_manifest[x]->file));
            write(new_manifest_file, " ", 1);
            write(new_manifest_file, server_manifest[x]->hash, strlen(server_manifest[x]->hash));
            write(new_manifest_file, "\n", 1);
        }
        x++;
    }

    /*delete the commit file*/
    int unl = unlink(pserver);

    /*tell the client that push was successful*/
    block_write(clientSoc, "32:Server has successfully pushed.\0", 35);
}

//=============================== COMMIT ======================
void create_commit_file(char *buffer, int clientSoc)
{
    /*get project name and check if the project exists in the server*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        free(project_name);
        int n = write(clientSoc, "ERROR project not in the server.\n", 36);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*make the pending commits folder - stores each commit for each client*/
    char* pending_commits = (char*)malloc(strlen(project_name)+strlen("/pending-commits\0")*sizeof(char));
    pending_commits[0]='\0';
    strcat(pending_commits, project_name);
    strcat(pending_commits, "/pending-commits\0"); 
    mkdir_recursive(pending_commits);
    int ch = chmod(pending_commits, 0775);
    if(ch<0) printf("ERROR set permission error.\n");

    /*make commit file*/
    char* hostname = get_host_name();
    char* pserver = (char*)malloc(strlen(project_name)+strlen(hostname)+strlen("/pending-commits/.Commit-"));
    pserver[0]='\0';
    strcat(pserver, project_name); 
    strcat(pserver, "/pending-commits/.Commit-");
    strcat(pserver, hostname);
    int commitFile = open(pserver, O_WRONLY | O_CREAT | O_TRUNC, 0775);
    if (commitFile < 0)
    {
        printf("ERROR unable to make commit file: %s\n", strerror(errno));
    }

    /*write the commit file content to the file*/
    bcount += (strlen(project_name) + 1);
    int count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    char *size = getSubstring(bcount, buffer, count - bcount);
    bcount += (strlen(size) + 1);
    char *file_content = (char *)malloc(atoi(size) * sizeof(char));
    int i = 0;
    while (i < atoi(size))
    {
        file_content[i] = buffer[bcount + i];
        i++;
    }
    file_content[atoi(size)] = '\0';
    write(commitFile, file_content, atoi(size));

    /*tell client everything is good*/
    block_write(clientSoc, "33:Server has made the commit file.\0", 36);
}

//===============================GET MANIFEST======================
/*Sends the contents of the manifest to the client.*/
int send_manifest(char *project_name, int clientSoc)
{
    printf("Sending Manifest to client\n");
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(project_name);
    if (dir == NULL)
    {
        return -1;
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", project_name, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type == DT_DIR)
        {
            int result = send_manifest(path, clientSoc);
        }
        else
        {
            if (strcmp(d->d_name, ".Manifest") == 0)
            {
                /*add the length of the full commmand to the front of the protocol*/
                char *manifest_data = getFileContent(path, "");
                //printf("%s\n", manifest_data);
                int m_len = strlen(manifest_data);
                char *m_len_str = to_Str(m_len);
                char *new_mdata = (char *)malloc(m_len + strlen(m_len_str) + 2 * sizeof(char));
                new_mdata[0] = '\0';
                strcat(new_mdata, m_len_str);
                strcat(new_mdata, ":");
                strcat(new_mdata, manifest_data);
                //printf("%s\n", new_mdata);
                /*write to the client*/
                int n = write(clientSoc, new_mdata, strlen(new_mdata));
                free(m_len_str);
                free(manifest_data);
                free(new_mdata);
                if (n < 0)
                {
                    printf("ERROR writing to client.\n");
                    closedir(dir);
                    return -1;
                }
                else
                {
                    closedir(dir);
                    return 0;
                }
            }
        }
    }
    closedir(dir);
    return -2;
}

/*Gets the project manifest that exists on server side.*/
void fetchServerManifest(char *buffer, int clientSoc)
{
    /*parse through the buffer*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    /*Error checking*/
    if (foundProj == 0)
    {
        free(project_name);
        int n = write(clientSoc, "44:ERROR the project does not exist on server.\n", 40);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*Call method to write to the client socket.*/
    int s = send_manifest(project_name, clientSoc);

    /*More error checking*/
    if (s == -1)
    {
        printf("ERROR sending Manifest to client./\n");
    }
    else if (s == -2)
    {
        printf("ERROR cannot find Manifest file./\n");
    }
    free(project_name);
    //free(pserver);
}
/* Returns true if file exists in project */
Boolean fileExists(char *fileName)
{
    DIR *dir = opendir(fileName);
    if (dir != NULL)
    {
        closedir(dir);
        return false;
    }
    closedir(dir);
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        //printf("Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        return false;
    }
    close(fd);
    return true;
}
void fetchFile(char *buffer, int clientSoc, char *command)
{
    printf("Sending File to client\n");
    char *cmd = strtok(buffer, ":");
    char *plens = strtok(NULL, ":");
    char *file_name = strtok(NULL, ":");
    printf("%s", file_name);
    int foundFile = fileExists(file_name);
    if (!foundFile)
    {
        free(file_name);
        int n = write(clientSoc, "40:ERROR the file does not exist on server.\n", 40);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }
    char *content = getFileContent(file_name, "");
    int contentLen = strlen(content);
    int digLen = digits(contentLen);
    int totalLen = digLen + contentLen + 1;
    char *send = (char *)malloc(sizeof(char *) * totalLen + 1);
    send[0] = '\0';
    char *messageLen = to_Str(contentLen);
    strcat(send, messageLen);
    strcat(send, ":");
    strcat(send, content);
    printf("%s\n", content);
    printf("%s\n", send);
    block_write(clientSoc, send, totalLen);
}

//=============================== DESTROY ======================
/*Returns 0 on success and -1 on fail on removing all files and directories given a path.*/
int remove_directory(char *dirPath)
{
    int r = -1;
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        printf("ERROR this is not a directory.\n");
        return -1;
    }
    r = 0;
    while (!r && (d = readdir(dir)) != NULL)
    {
        int r2 = -1;
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type == DT_DIR)
        {
            r2 = remove_directory(path);
        }
        else
        {
            r2 = unlink(path);
        }
        r = r2;
    }
    closedir(dir);
    if (r == 0)
    {
        r = rmdir(dirPath);
    }
    return 0;
}

/*Remove a project from the server side.*/
void destroyProject(char *buffer, int clientSoc)
{
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":"); // "12"
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens); //12
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    /*Error checking*/
    if (foundProj == 0)
    {
        free(project_name);
        int n = write(clientSoc, "ERROR the project does not exist on server.\n", 40);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    /*Call remove_directory*/
    char *pserver = (char *)malloc(strlen(project_name));
    pserver[0] = '\0';
    strcat(pserver, project_name);
    int r = remove_directory(pserver);

    /*Send message to client*/
    if (r < 0)
        printf("ERROR traversing directory.\n");
    else
    {
        free(project_name);
        int n = write(clientSoc, "Successfully destroyed project.\n", 33);
        if (n < 0)
            printf("ERROR writing to the client.\n");
        return;
    }

    free(pserver);
}

//=============================== CHECKOUT ======================
/*Increments the global counter- cmd_count*/
void inc_command_length(char *dirPath)
{
    /*traverse the directory*/
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        printf("ERROR opening directory.\n");
        return;
    }
    else
    {
        cmd_count += (digits(strlen(dirPath)));
        cmd_count += (strlen(dirPath));
        cmd_count += 4;
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type != DT_DIR)
        {
            /*for a file*/
            cmd_count += (digits(strlen(path)));
            cmd_count += (strlen(path));
            cmd_count += 6;

            /*for the file content*/
            char *data_client = getFileContent(path, "C");
            cmd_count += (digits(strlen(data_client)));
            cmd_count += (strlen(data_client));
            cmd_count += 2;
        }
        else
        {
            inc_command_length(path);
        }
    }
    closedir(dir);
}

/*Returns an char** array that stores all the things to be concatenated together.*/
char *checkoutProject(char *command, char *dirPath, int clientSoc)
{
    /*traverse the directory*/
    char path[4096];
    struct dirent *d;
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        printf("ERROR opening directory.\n");
        return NULL;
    }
    else
    {
        char f[2] = "P\0";
        char *new_dirpath = (char *)malloc(strlen(dirPath) + 2 * sizeof(char));
        check_malloc_null(new_dirpath);
        int i = 0;
        while (i < 2)
        {
            new_dirpath[i] = f[i];
            i++;
        }
        strcat(new_dirpath, dirPath);
        char *length_of_path = to_Str(strlen(new_dirpath));
        strcat(command, length_of_path);
        strcat(command, ":");
        strcat(command, new_dirpath);
        strcat(command, ":");
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (d->d_type != DT_DIR)
        {
            /*for a file*/
            char f[4] = "F./\0";
            char *new_path = (char *)malloc(strlen(path) + 4 * sizeof(char));
            check_malloc_null(new_path);
            int i = 0;
            while (i < 4)
            {
                new_path[i] = f[i];
                i++;
            }
            strcat(new_path, path);
            char *length_of_path = to_Str(strlen(new_path));
            strcat(command, length_of_path);
            strcat(command, ":");
            strcat(command, new_path);
            strcat(command, ":");

            /*for the file content*/
            char *data_client = getFileContent(path, "C");
            char *length_of_data = to_Str(strlen(data_client));
            strcat(command, length_of_data);
            strcat(command, ":");
            strcat(command, data_client);
            strcat(command, ":");
        }
        else
        {
            command = checkoutProject(command, path, clientSoc);
        }
    }

    closedir(dir);
    return command;
}

//============================= CREATE ===================================
/*Create project folder.*/
void createProject(char *buffer, int clientSoc)
{
    /*parse through the buffer*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    /*create project in server*/
    if (strcmp(cmd, "create") == 0)
    {
        /*check to see if project already exists on server*/
        if (foundProj == 1)
        {
            free(project_name);
            int n = write(clientSoc, "ERROR the project already exists on server.\n", 30);
            if (n < 0)
                printf("ERROR writing to the client.\n");
            return;
        }

        /*make project folder and set permissions*/
        char *pserver = (char *)malloc(strlen(project_name) + strlen("/.Manifest"));
        pserver[0] = '\0';
        strcat(pserver, project_name);
        mkdir_recursive(pserver);
        int ch = chmod(pserver, 0775);
        if (ch < 0)
            printf("ERROR set permission error.\n");

        /*make manifest file*/
        strcat(pserver, "/.Manifest");
        int manifestFile = open(pserver, O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if (manifestFile < 0)
        {
            printf("ERROR unable to make manifestFile: %s\n", strerror(errno));
        }
        write(manifestFile, "1\n", 2);
    }
    else if (strcmp(cmd, "checkout") == 0)
    {
        if (foundProj == 0)
        {
            free(project_name);
            int n = write(clientSoc, "ERROR project not in the server.\n", 36);
            if (n < 0)
                printf("ERROR writing to the client.\n");
            return;
        }
    }
    /*send string to client to make project*/
    cmd_count += (strlen(cmd) + 1);
    inc_command_length(project_name);
    char *command = (char *)malloc(cmd_count * sizeof(char));
    command[0] = '\0';
    strcat(command, cmd);
    strcat(command, ":");
    command = checkoutProject(command, project_name, clientSoc);

    /*add the length of the full commmand to the front of the protocol*/
    int cmd_length = strlen(command);
    char *cmd_len_str = to_Str(cmd_length);
    char *new_command = (char *)malloc(strlen(cmd_len_str) + cmd_length + 1 * sizeof(char));
    new_command[0] = '\0';
    strcat(new_command, cmd_len_str);
    strcat(new_command, ":");
    strcat(new_command, command);

    /*write command to client*/
    int n = write(clientSoc, new_command, strlen(new_command));
    if (n < 0)
        printf("ERROR writing to the client.\n");
    else
        printf("Success writing to the client.\n");

    /*free stuff*/
    free(command);
    free(new_command);
}

//=============================== READ CLIENT MESSAGE ======================
/*Parse through the client command.*/
void parseRead(char *buffer, int clientSoc)
{
    if (strstr(buffer, ":") != 0)
    {
        //figure out the command
        char *command = getCommand(buffer);
        //printf("Command: %s\n", command);
        if (strcmp(command, "create") == 0 || strcmp(command, "checkout") == 0)
        {
            createProject(buffer, clientSoc);
        }
        else if (strcmp(command, "destroy") == 0)
        {
            destroyProject(buffer, clientSoc);
        }
        else if (strcmp(command, "manifest") == 0)
        {
            fetchServerManifest(buffer, clientSoc);
        }
        else if (strcmp(command, "commit") == 0)
        {
            create_commit_file(buffer, clientSoc);
        }
        else if(strcmp(command, "push")==0)
        {
            push_commits(buffer, clientSoc);
        }
        else if(strcmp(command, "rollback")==0)
        {
            rollback(buffer, clientSoc);
        }
        else if (strcmp(command, "currentversion") == 0)
        {
            char *current_version = get_current_version(buffer, clientSoc);
            int n = write(clientSoc, current_version, strlen(current_version));
        }
        else if (strcmp(command, "sendfile") == 0)
        {
            fetchFile(buffer, clientSoc, command);
        }
        else
        {
            printf("Have not implemented this command yet!\n");
        }
        free(command);
    }
}

//============================ NETWORKING ==================================
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
    } while (buffer[readIn - 1] != ':' && status > 0);
    char *num = (char *)malloc(sizeof(char) * strlen(buffer));
    strncpy(num, buffer, strlen(buffer) - 1);
    num[strlen(buffer) - 1] = '\0';
    free(buffer);
    int len = atoi(num); // - strlen(num) - 1;
    free(num);
    return len;
}
void *mainThread(void *arg)
{

    int clientSock = *((int *)arg);
    // /* Code to infinite read from server and disconnect when client sends DONE*/
    while (1)
    {
        int len = read_len_message(clientSock);
        printf("Recieved Message of Length: %d\n", len);
        char *buffer = block_read(clientSock, len);
        //printf("Message from Client: %s\n", buffer);
        if (strcmp(buffer, "Done") == 0)
        {
            printf("Client Disconnected\n");
            free(buffer);
            break;
        }
        else
        {
            /*Parse through the buffer*/
            parseRead(buffer, clientSock);
        }
        free(buffer);
    }
    // do I close server listening socket? Idts
    printf("Server Disconnected\n");
    close(clientSock);
    pthread_exit(NULL);
}
void set_up_connection(char *port)
{
    int sockfd, clientSoc;
    struct sockaddr_in serv_addr, cli_addr;
    /* Set up Listening Socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        printf("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    int portno = atoi(port);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        printf("ERROR on binding");
    listen(sockfd, 10);
    printf("Server Listening");
    pthread_t tid[10];
    int i = 0;
    while (1)
    {
        /* Accept Client Connection = Blocking command */
        int clilen = sizeof(cli_addr);
        clientSoc = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (clientSoc < 0)
            printf("ERROR on accept");

        // /* Exchange Initial Messages */
        int len = read_len_message(clientSoc);
        char *buffer = block_read(clientSoc, len);
        printf("%s\n", buffer);
        block_write(clientSoc, "40:Client successfully connected to Server!", 43);
        free(buffer);
        if (pthread_create(&tid[i], NULL, mainThread, &clientSoc) != 0)
            printf("Failed to create thread\n");
    }
}

/* This method disconnects from client if necessary in the future*/
void disconnectServer(int fd)
{
    write(fd, "Done", 4);
    close(fd);
}

// ===================  MAIN METHOD  ================================================================================
int main(int argc, char **argv)
{
    int clientSock;
    int n;
    if (argc < 2)
    {
        printf("ERROR: No port provided\n");
        exit(1);
    }

    set_up_connection(argv[1]);

    // things to consider: do a nonblocking read, disconnect server from client
    return 0;
}