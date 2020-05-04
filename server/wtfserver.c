#include "header-files/helper.h"
#include "header-files/record.h"

/*Create a struct that holds the project name and the mutex. Basically a hashmap that links project name with a mutex.*/

int cmd_count = 0;

MutexArray *array_of_mutexes[20];
int project_count = 0;
int array_of_mutexes_size = 20;

/*Returns the corresponding mutex to the project name*/
pthread_mutex_t find_mutex(char *pname)
{
    int i = 0;
    while (i < array_of_mutexes_size)
    {
        if (array_of_mutexes[i] == NULL)
        {
            i++;
            continue;
        }
        else if (array_of_mutexes[i]->projectName == NULL)
        {
            i++;
            continue;
        }
        if (strcmp(array_of_mutexes[i]->projectName, pname) == 0)
        {
            return array_of_mutexes[i]->mutex;
        }
        i++;
    }
}

//=============================== HISTORY ======================
void save_history(char *commitData, char *projectName, char *version)
{
    char *filePath = (char *)malloc(strlen(projectName) + strlen("/.History") * sizeof(int));
    filePath[0] = '\0';
    strcat(filePath, projectName);
    strcat(filePath, "/.History");
    int fd = open(filePath, O_WRONLY | O_CREAT | O_APPEND, 00600);
    if (fd == -1)
    {
        close(fd);
        printf("[SERVER] Fatal Error: Could not open History file");
        return;
    }
    //write(fd, "Project version: ", strlen("Project version: "));
    // write(fd, version, strlen(version));
    write(fd, commitData, strlen(commitData));
    close(fd);
    free(filePath);
}

//=============================== ROLLBACK ======================

//23:projectname:13
void rollback(char *buffer, int clientSoc)
{
    /*get project name*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);

    /*check that the project exists on the server*/
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        free(project_name);
        block_write(clientSoc, "33:ERROR project not in the server.\n", 36);
        return;
    }

    /*Get the requested version number*/
    bcount += (strlen(project_name) + 1);
    int count = bcount; //17
    while (buffer[count] != ':')
    {
        count++;
    }
    //count = 18
    int nlen = count - bcount; //1
    bcount = (count + 1);
    int req_version = atoi(getSubstring(bcount, buffer, nlen));

    /*Traverse history folder - if the version number is greater then the requested then delete that version*/
    char *req_version_str = to_Str(req_version);
    if (strcmp(req_version_str, "") == 0)
    {
        req_version_str = "0\0";
    }
    char *tar_name = (char *)malloc(strlen(project_name) + digits(req_version) + strlen(".tar") + 1 * sizeof(char));
    tar_name[0] = '\0';
    strcat(tar_name, project_name);
    strcat(tar_name, "-");
    strcat(tar_name, req_version_str);
    strcat(tar_name, ".tar");
    Boolean found = search_tars(tar_name);

    if (found == false)
    {
        block_write(clientSoc, "58:ERROR could not find version. Choose a different version.\n", 61);
        return;
    }

    char *tar_name_2 = (char *)malloc(strlen("history/") + strlen(project_name) + digits(req_version) + strlen(".tar") + 1 * sizeof(char));
    tar_name_2[0] = '\0';
    strcat(tar_name_2, "history/");
    strcat(tar_name_2, project_name);
    strcat(tar_name_2, "-");
    strcat(tar_name_2, req_version_str);
    strcat(tar_name_2, ".tar");

    remove_directory(project_name);

    untarFile(tar_name_2);

    /*remove the files in pending commits*/
    char path[4096];
    struct dirent *d;
    char *pend_commits_folder = (char *)malloc(strlen(project_name) + strlen("/pending-commits"));
    pend_commits_folder[0] = '\0';
    strcat(pend_commits_folder, project_name);
    strcat(pend_commits_folder, "/pending-commits");
    remove_directory(pend_commits_folder);

    remove_new_versions(project_name, req_version);

    /*Send a success to the client.*/
    block_write(clientSoc, "37:Server has successfully rolled back.\n", 40);
}

//=============================== PUSH ======================

void expire_pending_commits(char *project_name, char *good_commit)
{
    char path[4096];
    struct dirent *d;

    char *pserver = (char *)malloc(strlen(project_name) + strlen("/pending-commits"));
    pserver[0] = '\0';
    strcat(pserver, project_name);
    strcat(pserver, "/pending-commits");

    DIR *dir = opendir(pserver);
    if (dir == NULL)
    {
        printf("[SERVER] ERROR this is not a directory.\n");
        return;
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", pserver, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0)
            continue;
        if (strcmp(d->d_name, good_commit) == 0)
        {
            continue;
        }
        else
        {
            int u = unlink(path);
        }
    }
    closedir(dir);
}

//push:23:projectname:234:blahblahcommintcontent
void push_commits(char *buffer, int clientSoc)
{
    /*get project name*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    bcount += (strlen(project_name) + 1);

    /*check if the project exists in the server*/
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        expire_pending_commits(project_name, "");
        //free(project_name);
        block_write(clientSoc, "33:ERROR project not in the server.\n", 36);
        return;
    }

    /*mutex stuff*/
    pthread_mutex_t m = find_mutex(project_name);
    // pthread_mutex_unlock(&m);
    pthread_mutex_lock(&m);

    /*get client host name - to find commit file*/
    int count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    int nlen = count - bcount;
    char *size_of_hostname = getSubstring(bcount, buffer, nlen);
    bcount += (strlen(size_of_hostname) + 1);
    char *hostname = getSubstring(bcount, buffer, atoi(size_of_hostname));
    bcount += (strlen(hostname) + 1);
    free(size_of_hostname);

    /*Get the commit file data*/
    count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    nlen = count - bcount;
    char *size = getSubstring(bcount, buffer, nlen);
    bcount += (strlen(size) + 1);
    char *file_content = getSubstring(bcount, buffer, atoi(size));
    bcount += (strlen(file_content) + 1);
    //free(size);

    /*Get server commit file and compare with the client commit file*/
    // char *hostname = get_host_name();
    char *pserver = (char *)malloc(strlen(project_name) + strlen(hostname) + strlen("/pending-commits/.Commit-"));
    pserver[0] = '\0';
    strcat(pserver, project_name);
    strcat(pserver, "/pending-commits/.Commit-");
    strcat(pserver, hostname);
    char *server_file_content = getFileContent(pserver, "");

    /*server commit file doesn't exist*/
    if (server_file_content == NULL)
    {
        expire_pending_commits(project_name, "");
        block_write(clientSoc, "63:Push command failed: The server does not have the commit file.\n", 66);
        return;
    }

    /*server and client commits do not match*/
    if (strcmp(file_content, server_file_content) != 0)
    {
        if (strcmp(server_file_content, "") == 0)
        {
            printf("[SERVER] ERROR the server commit file is empty.\n");
        }
        else
        {
            printf("[SERVER] ERROR the client and sever commit files do not match.\n");
        }
        expire_pending_commits(project_name, "");
        block_write(clientSoc, "66:Push command failed: Server and client commit files do not match.\n", 69);
        return;
    }

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
    //  free(size2);

    /*Get the current server manifest that is in the folder*/
    char *manifestpath = (char *)malloc(strlen(project_name) + strlen("./Manifest") * sizeof(char));
    manifestpath[0] = '\0';
    strcat(manifestpath, project_name);
    strcat(manifestpath, "/.Manifest");
    char *manifest_data = getFileContent(manifestpath, "");
    Record **server_manifest = create_record_struct(manifest_data, num, 'M');
    int server_manifest_size = getRecordStructSize(server_manifest);
    // free(manifest_data);

    /*Get the client manifest- make it into a record struct*/
    Record **client_manifest = create_record_struct(file_content2, 0, 'M');
    int client_manifest_size = getRecordStructSize(server_manifest);
    //  free(file_content2);

    /*save this push to the history*/
    char* version_num = server_manifest[0]->version;
    save_history(server_file_content, project_name, version_num);

    /*Expire the other pending commits*/
    char *good_commit = (char *)malloc(strlen(hostname) + strlen(".Commit-"));
    good_commit[0] = '\0';
    strcat(good_commit, ".Commit-");
    strcat(good_commit, hostname);
    expire_pending_commits(project_name, good_commit);

    /*make a history folder to store all the old commits*/
    mkdir("history", 0775);

    /*tar this current version of the project*/
    count = 0;
    Boolean not_used_name = false;
    char *tar_name = NULL;
    while (not_used_name != true)
    {
        char *number = to_Str(count);
        if (strcmp(number, "") == 0)
            number = "0\0";
        char *tar_name = (char *)malloc(strlen(project_name) + strlen(number) + strlen(".tar") + 1 * sizeof(char));
        tar_name[0] = '\0';
        strcat(tar_name, project_name);
        strcat(tar_name, "-");
        strcat(tar_name, number);
        strcat(tar_name, ".tar");
        Boolean found = search_tars(tar_name);
        if (found == false)
        {
            char *tar_name_2 = (char *)malloc(strlen(project_name) + strlen(number) + 1 * sizeof(char));
            tar_name_2[0] = '\0';
            strcat(tar_name_2, project_name);
            strcat(tar_name_2, "-");
            strcat(tar_name_2, number);
            tarFile(tar_name_2, project_name);
            not_used_name = true;
        }
        else
        {
            // free(number);
            // free(tar_name);
        }
        count++;
    }

    /*do the stuff that is in the commit-modify the manifest too*/
    Record **active_commit = create_record_struct(file_content, 0, 'C');
    int active_commit_size = getRecordStructSize(active_commit);
    int commit_size = number_of_lines(file_content);
    int x = 0;
    while (x < commit_size)
    {
        if (strcmp(active_commit[x]->version, "M") == 0)
        {
            //modify code
            char *filepath = active_commit[x]->file;
            Record *manifest_rec = search_record(server_manifest, filepath);
            Record *client_manifest_rec = search_record(client_manifest, filepath);
            if (manifest_rec != NULL)
            {
                manifest_rec->version = to_Str(atoi(client_manifest_rec->version) + 1);
                manifest_rec->hash = active_commit[x]->hash;
            }
            else
            {
                expire_pending_commits(project_name, "");
                printf("[SERVER] ERROR could not find the file in the manifest. Update?\n");
                block_write(clientSoc, "54:Push command failed: The file is not in the manifest.\n", 57);
                return;
            }
            /*Get the file from the client and read what the new contents of the file are!*/
            char *newContent = fetch_file_from_client(filepath, clientSoc);
            if (newContent == NULL)
            {
                expire_pending_commits(project_name, "");
                printf("[SERVER] Fatal Error: Could not find the path %s in the client.\n", filepath);
                block_write(clientSoc, "60:Push command failed: Could not find the file in the client!\n", 63);
                return;
            }
            // mkdir_recursive(filepath); //for subdirectories ^_^ (hopefully it works)
            int fd = open(filepath, O_WRONLY | O_TRUNC);
            if (fd == -1)
            {
                close(fd);
                expire_pending_commits(project_name, "");
                printf("[SERVER] Fatal Error: Could not open file");
                block_write(clientSoc, "42:Push command failed: Could not open file!\n", 45);
                return;
            }
            if (strcmp(newContent, "empty") != 0)
            {
                block_write(fd, newContent, strlen(newContent));
            }
            close(fd);
        }
        else if (strcmp(active_commit[x]->version, "A") == 0)
        { //add file
            /*make commit file*/
            char *filepath = active_commit[x]->file;

            /*handle subdirectories*/
            char *subdirPath = extract_path(filepath);
            char *projTemp = (char *)malloc(sizeof(char) * strlen(project_name) + 2);
            strcpy(projTemp, project_name);
            strcat(project_name, "/");
            projTemp[strlen(project_name) + 1] = '\0';
            if (strcmp(subdirPath, projTemp) != 0)
            {
                mkdir_recursive(subdirPath);
            }
            free(subdirPath);
            free(projTemp);

            int new_file = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
            if (new_file < 0)
            {
                close(new_file);
                expire_pending_commits(project_name, "");
                printf("[SERVER] ERROR unable to make new file: %s\n", strerror(errno));
                block_write(clientSoc, "46:Push command failed: Unable to make new file!\n", 49);
                return;
            }
            close(new_file);
            /*add the new file to the manifest*/
            Record *manifest_rec = search_record(server_manifest, filepath);
            if (manifest_rec != NULL)
            {
                expire_pending_commits(project_name, "");
                printf("[SERVER] ERROR already exists in manifest.\n");
                block_write(clientSoc, "58:Push command failed: File already exists in the manifest!\n", 61);
                return;
            }
            else
            {
                Record *new_manifest_rec = (Record *)malloc(sizeof(Record));
                new_manifest_rec->version = "0";
                new_manifest_rec->file = filepath;
                new_manifest_rec->hash = active_commit[x]->hash;
                int m = 0;
                while (server_manifest[m] != NULL && m < server_manifest_size)
                {
                    m++;
                }
                if (server_manifest[m] == NULL && m != server_manifest_size)
                {
                    server_manifest[m] = new_manifest_rec;
                }
            }
            char *newContent = fetch_file_from_client(filepath, clientSoc);
            if (newContent == NULL)
            {
                expire_pending_commits(project_name, "");
                printf("[SERVER] Fatal Error: Could not find the path %s in the client.\n", filepath);
                block_write(clientSoc, "60:Push command failed: Could not find the file in the client!\n", 63);
                return;
            }
            mkdir_recursive(filepath);
            // write content to file
            int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 00600);
            if (fd == -1)
            {
                close(fd);
                expire_pending_commits(project_name, "");
                printf("[SERVER] Fatal Error: Could not open file");
                block_write(clientSoc, "42:Push command failed: Could not open file!\n", 45);
                return;
            }
            if (strcmp(newContent, "empty") != 0)
            {
                block_write(fd, newContent, strlen(newContent));
            }
            close(fd);
        }
        else if (strcmp(active_commit[x]->version, "D") == 0)
        {
            char *filepath = active_commit[x]->file;
            int y = 1;
            int size = getRecordStructSize(server_manifest);
            while (y < size)
            {
                if (server_manifest[y] != NULL && server_manifest[y]->file != NULL && strcmp(server_manifest[y]->file, filepath) == 0)
                {
                    free(server_manifest[y]->version);
                    free(server_manifest[y]->file);
                    free(server_manifest[y]->hash);
                    free(server_manifest[y]);
                    server_manifest[y] = NULL;
                }
                y++;
            }
            int r = remove(filepath);
        }
        x++;
    }

    /*re-make the manifest file*/
    int new_manifest_file = open(manifestpath, O_WRONLY | O_CREAT | O_TRUNC, 0775);
    x = 0;
    while (x < server_manifest_size)
    {
        if (x == 0)
        {
            char *new_vers = to_Str(atoi(server_manifest[0]->version) + 1);
            write(new_manifest_file, new_vers, strlen(new_vers));
            write(new_manifest_file, "\n", 1);
            free(new_vers);
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
    close(new_manifest_file);

    /*delete the commit file*/
    int unl = unlink(pserver);

    /*tell the client that push was successful*/
    block_write(clientSoc, "32:Server has successfully pushed.\n", 35);

    pthread_mutex_unlock(&m);
}

//=============================== COMMIT ======================
void create_commit_file(char *buffer, int clientSoc)
{
    /*get project name*/
    int bcount = 0;
    char *cmd = strtok(buffer, ":");
    bcount += strlen(cmd) + 1;
    char *plens = strtok(NULL, ":");
    bcount += strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);

    /*Check that the project exists on server*/
    int foundProj = search_proj_exists(project_name);
    if (foundProj == 0)
    {
        //free(project_name);
        block_write(clientSoc, "33:ERROR project not in the server.\n", 36);
        return;
    }

    /*make the pending commits folder - stores each commit for each client*/
    char *pending_commits = (char *)malloc(strlen(project_name) + strlen("/pending-commits\0") * sizeof(char));
    pending_commits[0] = '\0';
    strcat(pending_commits, project_name);
    strcat(pending_commits, "/pending-commits\0");
    mkdir_recursive(pending_commits);
    int ch = chmod(pending_commits, 0775);
    if (ch < 0)
    {
        printf("[SERVER] ERROR set permission error.\n");
    }

    /*Get the client host name*/
    bcount += (strlen(project_name) + 1);
    int count = bcount;
    while (buffer[count] != ':')
    {
        count++;
    }
    char *size_of_hostname = getSubstring(bcount, buffer, count - bcount);
    bcount += (strlen(size_of_hostname) + 1);
    char *hostname = getSubstring(bcount, buffer, atoi(size_of_hostname));
    bcount += (strlen(hostname) + 1);

    /*make commit file*/
    //printf("[SERVER] %s\n", hostname);
    char *pserver = (char *)malloc(strlen(project_name) + strlen(hostname) + strlen("/pending-commits/.Commit-"));
    pserver[0] = '\0';
    strcat(pserver, project_name);
    strcat(pserver, "/pending-commits/.Commit-");
    strcat(pserver, hostname);
    int commitFile = open(pserver, O_WRONLY | O_CREAT | O_TRUNC, 0775);
    if (commitFile < 0)
    {
        //free(project_name);
        //free(pending_commits);
        //free(pserver);
        block_write(clientSoc, "34:ERROR unable to make commit file.\n", 37);
        return;
    }

    /*write the commit file content to the file*/
    // bcount += (strlen(project_name) + 1);
    count = bcount;
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

    close(commitFile);

    block_write(clientSoc, "48:Successfully created commit file in the server.\n", 51);
}

//===============================GET MANIFEST======================
/*Sends the contents of the manifest to the client.*/
int send_manifest(char *project_name, int clientSoc)
{
    //printf("[SERVER] Sending Manifest to client\n");
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
                //printf("[SERVER] %s\n", manifest_data);
                int m_len = strlen(manifest_data);
                char *m_len_str = to_Str(m_len);
                char *new_mdata = (char *)malloc(m_len + strlen(m_len_str) + 2 * sizeof(char));
                new_mdata[0] = '\0';
                strcat(new_mdata, m_len_str);
                strcat(new_mdata, ":");
                strcat(new_mdata, manifest_data);
                //printf("[SERVER] %s\n", new_mdata);
                /*write to the client*/
                int n = write(clientSoc, new_mdata, strlen(new_mdata));
                free(m_len_str);
                free(manifest_data);
                free(new_mdata);
                if (n < 0)
                {
                    printf("[SERVER] ERROR writing to client.\n");
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
        block_write(clientSoc, "44:ERROR the project does not exist on server.\n", 47);
        return;
    }

    /*Call method to write to the client socket.*/
    int s = send_manifest(project_name, clientSoc);

    /*More error checking*/
    if (s == -1)
    {
        printf("[SERVER] ERROR sending Manifest to client./\n");
    }
    else if (s == -2)
    {
        printf("[SERVER] ERROR cannot find Manifest file./\n");
    }
    free(project_name);
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
        //printf("[SERVER] Fatal Error: %s named %s\n", strerror(errno), file);
        //close(fd);
        return false;
    }
    close(fd);
    return true;
}
void fetchFile(char *buffer, int clientSoc, char *command)
{
    char *cmd = strtok(buffer, ":");
    char *plens = strtok(NULL, ":");
    char *file_name = strtok(NULL, ":");
    //printf("[SERVER] Sending file to client: %s", file_name);
    int foundFile = fileExists(file_name);
    if (!foundFile)
    {
        block_write(clientSoc, "40:ERROR the file does not exist on server.\n", 43);
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
    //printf("[SERVER] %s\n", content);
    //printf("[SERVER] %s\n", send);
    block_write(clientSoc, send, totalLen);

    free(content);
    free(messageLen);
    free(send);
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
        printf("[SERVER] ERROR this is not a directory.\n");
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
        int n = write(clientSoc, "44:ERROR the project does not exist on server.\n", 47);
        if (n < 0)
            printf("[SERVER] ERROR writing to the client.\n");
        return;
    }

    pthread_mutex_t m = find_mutex(project_name);
    pthread_mutex_lock(&m);

    /*Call remove_directory*/
    char *pserver = (char *)malloc(strlen(project_name));
    pserver[0] = '\0';
    strcat(pserver, project_name);

    // free(array_of_mutexes[project_count]->projectName);

    int r = remove_directory(pserver);

    free(project_name);
    free(pserver);

    /*Send message to client*/
    if (r < 0)
    {
        block_write(clientSoc, "27:Failed to destroy project.\n", 31);
    }
    else
    {
        block_write(clientSoc, "32:Successfully destroyed project.\n", 35);
    }
    return;
    pthread_mutex_unlock(&m);
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
        printf("[SERVER] ERROR opening directory.\n");
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
            free(data_client);
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
        printf("[SERVER] ERROR opening directory.\n");
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

        free(length_of_path);
        free(new_dirpath);
    }
    while ((d = readdir(dir)) != NULL)
    {
        snprintf(path, 4096, "%s/%s", dirPath, d->d_name);
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".git") == 0 || strcmp(d->d_name, "pending-commits") == 0 || strcmp(d->d_name, "history") == 0)
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

            free(new_path);
            free(length_of_path);

            /*for the file content*/
            char *data_client = getFileContent(path, "C");
            char *length_of_data = to_Str(strlen(data_client));
            strcat(command, length_of_data);
            strcat(command, ":");
            strcat(command, data_client);
            strcat(command, ":");

            free(data_client);
            free(length_of_data);
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
    char *plens = strtok(NULL, ":");
    bcount += strlen(cmd) + 1 + strlen(plens) + 1;
    int pleni = atoi(plens);
    char *project_name = getSubstring(bcount, buffer, pleni);
    int foundProj = search_proj_exists(project_name);

    if (strcmp(cmd, "create") == 0)
    {
        /*check to see if project already exists on server*/
        if (foundProj == 1)
        {
            free(project_name);
            block_write(clientSoc, "34:ERROR the project already exists.\n", 37);
            return;
        }

        /*Add this project name to the mutex Array*/
        char *proj_name_mutex = (char *)malloc(sizeof(char) * strlen(project_name));
        strcpy(proj_name_mutex, project_name);
        array_of_mutexes[project_count]->projectName = proj_name_mutex;
        project_count++;

        /*make project folder and set permissions*/
        char *pserver = (char *)malloc(strlen(project_name) + strlen("/.Manifest") + 1);
        check_malloc_null(pserver);
        pserver[0] = '\0';
        strcat(pserver, project_name);
        mkdir_recursive(pserver);
        int ch = chmod(pserver, 0775);
        if (ch < 0)
        {
            block_write(clientSoc, "43:ERROR setting permissions for new project.\n", 46);
            return;
        }

        /*make manifest file*/
        strcat(pserver, "/.Manifest");
        int manifestFile = open(pserver, O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if (manifestFile < 0)
        {
            block_write(clientSoc, "36:ERROR unable to make manifest file.\n", 39);
            return;
        }
        write(manifestFile, "0\n", 2);
        close(manifestFile);
        free(pserver);

        char *pserver2 = (char *)malloc(strlen(project_name) + strlen("/.History") + 1);
        check_malloc_null(pserver2);
        pserver2[0] = '\0';
        strcat(pserver2, project_name);
        strcat(pserver2, "/.History");
        int historyFile = open(pserver2, O_WRONLY | O_CREAT | O_TRUNC, 0775);
        if (historyFile < 0)
        {
            block_write(clientSoc, "36:ERROR unable to make history file.\n", 39);
            return;
        }
        close(historyFile);
    }
    else if (strcmp(cmd, "checkout") == 0)
    {
        if (foundProj == 0)
        {
            free(project_name);
            block_write(clientSoc, "30:ERROR project does not exist.\n", 33);
            return;
        }
    }

    cmd_count = 0;
    cmd_count += (strlen(cmd) + 1);

    inc_command_length(project_name);
    char *command = (char *)malloc(cmd_count * sizeof(char));
    command[0] = '\0';
    strcat(command, cmd);
    strcat(command, ":");
    command = checkoutProject(command, project_name, clientSoc);
    if (command == NULL)
    {
        block_write(clientSoc, "25:ERROR creating protocol.\n", 28);
        return;
    }

    /*add the length of the full commmand to the front of the protocol*/
    int size = digits(strlen(command)) + 1 + strlen(command) + 2;
    char *new_command = (char *)malloc(size * sizeof(char));
    new_command[0] = '\0';
    char *slen = to_Str(strlen(command));
    strcat(new_command, slen);
    strcat(new_command, ":");
    strcat(new_command, command);
    new_command[size - 1] = '\0';

    /*write command to client*/
    int n = write(clientSoc, new_command, size);
    if (n < 0)
        printf("[SERVER] ERROR writing to the client.\n");

    /*free stuff*/
    free(project_name);
    free(slen);
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
        int strcount = 0;
        while (buffer[strcount] != ':')
        {
            strcount++;
        }
        char *command = (char *)malloc(strcount + 1 * sizeof(char));
        check_malloc_null(command);
        int i = 0;
        while (i < strcount)
        {
            command[i] = buffer[i];
            i++;
        }
        command[strcount] = '\0';

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
            return;
        }
        else if (strcmp(command, "push") == 0)
        {
            push_commits(buffer, clientSoc);
            return;
        }
        else if (strcmp(command, "rollback") == 0)
        {
            rollback(buffer, clientSoc);
        }
        // else if (strcmp(command, "currentversion") == 0)
        // {
        //     char *current_version = get_current_version(buffer, clientSoc, 'N');
        //     char *format_str = (char *)malloc(digits(strlen(current_version)) + 1 + strlen(current_version) * sizeof(char));
        //     format_str[0] = '\0';
        //     char *intToStr = to_Str(strlen(current_version));
        //     strcat(format_str, intToStr);
        //     strcat(format_str, ":");
        //     strcat(format_str, current_version);
        //     block_write(clientSoc, format_str, strlen(format_str));

        //     free(format_str);
        //     free(intToStr);
        // }
        else if (strcmp(command, "sendfile") == 0)
        {
            fetchFile(buffer, clientSoc, command);
        }
        else
        {
            printf("[SERVER] Have not implemented this command yet!\n");
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
    if (targetBytes == 0)
        return NULL;
    do
    {
        status = read(fd, buffer + readIn, targetBytes - readIn);
        readIn += status;
    } while (status > 0 && readIn < targetBytes);
    buffer[targetBytes] = '\0';
    if (readIn < 0)
        printf("[SERVER] ERROR reading from socket");
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
        printf("[SERVER] ERROR writing to socket");
}

int read_len_message(int fd)
{
    char buffer[50];
    bzero(buffer, 50);
    int status = 0;
    int readIn = 0;
    do
    {
        status = read(fd, buffer + readIn, 1);
        readIn += status;
    } while (buffer[readIn - 1] != ':' && status > 0);
    char num[20];
    int i = 0;
    while (i < strlen(buffer))
    {
        num[i] = buffer[i];
        i++;
    }
    num[strlen(buffer)] = '\0';
    int len = atoi(num);
    return len;
}

void clean_up_code()
{
    printf("[SERVER] Clean up your code before exiting.\n");
    exit(0);
}

/*Handing SIGINT*/
void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("[SERVER] received SIGINT\n");
        atexit(clean_up_code);
        exit(0);
    }
}

void *mainThread(void *arg)
{
    int clientSock = *((int *)arg);

    /* Code to infinite read from server and disconnect when client sends DONE*/
    while (1)
    {
        int len = read_len_message(clientSock);
        char *buffer = block_read(clientSock, len);
        if (buffer == NULL)
            continue;
        if (strcmp(buffer, "Done") == 0)
        {
            printf("[SERVER] Client Disconnected\n");
            free(buffer);
            break;
        }
        else if (strcmp(buffer, "Successfully connected to client.\n") == 0)
        {
            printf("[SERVER] %s\n", buffer);
            block_write(clientSock, "34:Successfully connected to server.\n", 37);
        }
        else
        {
            parseRead(buffer, clientSock);
        }
        free(buffer);
    }
    printf("[SERVER] Server Disconnected\n");
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
        printf("[SERVER] ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    int portno = atoi(port);

    /*bind socket to port*/
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[SERVER] Fatal Error: on binding");
        exit(0);
    }
    /*listen on port*/
    listen(sockfd, 10);
    printf("[SERVER] Server Listening\n");

    /*Initialize the mutex array structure*/
    // int num_of_projects = find_all_projects();
    // array_of_mutexes_size = num_of_projects;
    // char** projectNames = get_project_names(num_of_projects);
    //array_of_mutexes = (MutexArray*)(num_of_projects*sizeof(MutexArray));
    int i = 0;
    while (i < 20)
    {
        MutexArray *new_mutex_element = (MutexArray *)malloc(sizeof(MutexArray));
        new_mutex_element->projectName = NULL;
        pthread_mutex_init(&(new_mutex_element->mutex), NULL);
        array_of_mutexes[i] = new_mutex_element;
        i++;
    }

    /*For every client that connects - throw it into a new thread*/
    pthread_t tid[10];
    i = 0;
    while (1)
    {
        /*Handing SIGINT*/
        if (signal(SIGINT, sig_handler) == SIG_ERR)
        {
            printf("[SERVER] ERROR cannot catch SIGINT.\n");
        }

        /* Accept Client Connection = Blocking command */
        int clilen = sizeof(cli_addr);
        clientSoc = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (clientSoc < 0)
        {
            printf("[SERVER] ERROR on accept\n");
        }
        else
        {
            int error = pthread_create(&tid[i], NULL, mainThread, &clientSoc);
            if (error < 0)
            {
                printf("[SERVER] ERROR unable to create thread.\n");
            }
            //pthread_detach(tid[i]);
            i++;
        }
    }

    /*Join all the threads*/
    int j = 0;
    while (j < i)
    {
        pthread_join(tid[i], NULL);
    }

    /*Free and destroy the mutex array structure*/
    i = 0;
    while (i < 20)
    {
        if (array_of_mutexes[i]->projectName != NULL)
        {
            free(array_of_mutexes[i]->projectName);
        }
        pthread_mutex_destroy(&(array_of_mutexes[i]->mutex));
        free(array_of_mutexes[i]);
        i++;
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
        printf("[SERVER] ERROR: No port provided\n");
        exit(1);
    }

    set_up_connection(argv[1]);

    // things to consider: do a nonblocking read, disconnect server from client
    return 0;
}