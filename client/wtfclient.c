#include "header-files/helper.h"
#include "header-files/record.h"

void syncManifests(char* project_name, char* buffer){
    char *manifest = (char *)malloc(strlen(project_name) + strlen("./Manifest") * sizeof(char));
    manifest[0] = '\0';
    strcat(manifest, project_name);
    strcat(manifest, "/.Manifest");
    int new_manifest_file = open(manifest, O_WRONLY | O_CREAT | O_TRUNC, 0775);
    int w = write(new_manifest_file, buffer, strlen(buffer));
    if(w < 0)
        printf("ERROR writing to the manifest.\n");
}

//========================HASHING===================================================
unsigned char *getHash(char *s)
{
    unsigned char *digest = SHA256(s, strlen(s), 0);
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
        char *file_path = (char *)malloc(strlen(record_arr[i]->file) * sizeof(char));
        strcat(file_path, record_arr[i]->file);
        char *file_content = read_file(file_path);
        char *live_hash = getHash(file_content);
        live_hash_arr[i] = live_hash;
        i++;
    }
    return live_hash_arr;
}

//=================================== COMMIT ===================================
/* Removes the commit file from the client after push is called! */
void remove_commit_file(int sockfd, char *project_name)
{
    char *commit_path = (char *)malloc(strlen(project_name) + 1 + strlen("/.Commit") * sizeof(char));
    commit_path[0] = '\0';
    strcat(commit_path, project_name);
    strcat(commit_path, "/.Commit");
    int c = unlink(commit_path);
    if (c < 0)
        printf("ERROR could not delete the commit file!\n");
    free(commit_path);
}

/*Sends the commit file to the server.*/
void write_commit_file(int sockfd, char *project_name, char *server_record_data)
{
    //first check if the client has updates to be made
    char *pclient = (char *)malloc(strlen(project_name) + strlen("./Update") * sizeof(char));
    pclient[0] = '\0';
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
    char *pclient2 = (char *)malloc(strlen(project_name) + strlen("./Conflict") * sizeof(char));
    pclient2[0] = '\0';
    strcat(pclient2, project_name);
    strcat(pclient2, "/.Conflict");
    if (fileExists(pclient2) == true)
    {
        free(pclient2);
        printf("ERROR conflicts still exist. Please resolve conflicts first.\n");
        return;
    }
    // free(pclient2);

    // /*get server and client manifests*/
    char *pclient3 = (char *)malloc(strlen(project_name) + strlen("./Manifest") * sizeof(char));
    pclient3[0] = '\0';
    strcat(pclient3, project_name);
    strcat(pclient3, "/.Manifest");
    Record **server_manifest = create_record_struct(server_record_data, true);
    //printAllRecords(server_manifest);
    char *client_manifest_data = read_file(pclient3);
    Record **client_manifest = create_record_struct(client_manifest_data, true);
    //printAllRecords(server_manifest);
    int client_manifest_size = getRecordStructSize(client_manifest);
    int server_manifest_size = getRecordStructSize(server_manifest);

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

    /*create commit if it doesn't exist- otherwise just append to the old commit file*/
    int path_len = strlen("./Commit") + strlen(project_name);
    char *commit_path = (char *)malloc(path_len + 1 * sizeof(char));
    snprintf(commit_path, path_len + 1, "%s%s", project_name, "/.Commit");
    commit_path[path_len + 1] = '\0';
    int commit_fd = open(commit_path, O_WRONLY | O_CREAT | O_APPEND, 00600);

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
            write(commit_fd, live_hash_arr[i], strlen(live_hash_arr[i]));
            write(commit_fd, "\n", 1);
            /*increment the file version*/
            int client_file_version = atoi(client_manifest[i]->version);
            client_manifest[i]->version = to_Str(client_file_version + 1);
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
            write(commit_fd, server_manifest[i]->hash, strlen(server_manifest[i]->hash));
            write(commit_fd, "\n", 1);
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
    strcat(extended_commit_cmd, send_commit_to_server);
    int w = write(sockfd, extended_commit_cmd, strlen(extended_commit_cmd));
    if (w < 0)
        printf("ERROR writing to the server.\n");
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
    printf("Sending Message to Server: %s\n", command);
    block_write(sockfd, command, totalLen);
    int messageLen = read_len_message(sockfd);
    //printf("Receieved from Server a message of length: %d\n", messageLen);
    char *serverData = block_read(sockfd, messageLen);
    if (strstr(serverData, "ERROR") != NULL) // check if errored (project name does not exist on server)
    {
        printf("%s", serverData);
        return NULL;
    }
    //printf("%s\n", serverData);
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
    printf("%s\n", temp);
    return temp;
}
void upgrade(char *projectName, int sockfd)
{
    // check if there is  .Update file or .Conflict file
    char *updateFilePath = (char *)malloc(strlen(projectName) + 1 + 8);
    strcpy(updateFilePath, projectName);
    strcat(updateFilePath, "/.Update");
    char *conflictFilePath = (char *)malloc(strlen(projectName) + 1 + 10);
    strcpy(conflictFilePath, projectName);
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
            printf("Project is up to date\n");
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

        // sync manifest version numbers
        manifestClient[0]->version = serverRecords[0]->version;
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
                char *subdirPath = extract_path(updateRecords[x]->file);
                char *projTemp = (char *)malloc(sizeof(char) * strlen(projectName) + 2);
                strcpy(projTemp, projectName);
                strcat(projectName, "/");
                projTemp[strlen(projectName) + 1] = '\0';
                if (strcmp(subdirPath, projTemp) != 0)
                {
                    mkdir_recursive(subdirPath);
                }
                free(subdirPath);
                free(projTemp);
                // write content to file
                int fd = open(updateRecords[x]->file, O_WRONLY | O_CREAT | O_TRUNC, 00600);
                if (fd == -1)
                {
                    printf("Fatal Error: Could not open file");
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

//================ UPDATE METHODS============================
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
            //free(currentFileData);
            //free(currentHash);
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

//================= ADD/REMOVE METHODS =============================
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
                if(searchForProject(projectName) == true){
                    printf("ERROR project already exists!\n");
                    return;
                }
                mkdir_recursive(projectName);
                int ch = chmod(projectName, 0775);
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
}
//================  HISTORY ==================
void get_history_file(char *projectName, int sockfd)
{
    char *historyFilePath = (char *)malloc(strlen(projectName) + 1 + 9);
    strcpy(historyFilePath, projectName);
    strcat(historyFilePath, "/.History");
    char *newContent = fetch_file_from_server(historyFilePath, sockfd);
    if (newContent == NULL)
    {
        return;
    }
    printf("History for %s\n%s", projectName, newContent);
    printf("Hello!\n");
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
    //printf("%s", num);
    int len = atoi(num);
    free(num);
    return len;
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

    // /* Exchange initial connection messages*/
    block_write(sockfd, "34:Successfully connected to client.\n", 37);
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
void write_to_server(int sockfd, char *argv1, char *argv2, char *project_name)
{
    int clen = strlen(argv2) + digits(strlen(project_name)) + strlen(argv1) + 2;
    char *command = (char *)malloc(clen + 1 * sizeof(char));
    snprintf(command, clen + 1, "%s:%d:%s", argv1, strlen(project_name), argv2);

    char *new_command = (char *)malloc(strlen(command) + digits(strlen(command)) + 1 * sizeof(char));
    new_command[0] = '\0';
    strcat(new_command, to_Str((strlen(command))));
    strcat(new_command, ":");
    strcat(new_command, command);
    block_write(sockfd, new_command, strlen(new_command));
    
    free(command);
    free(new_command);
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
        write_to_server(sockfd, argv[1], argv[2], argv[2]);
        
        char *buffer = read_from_server(sockfd);
        if(strcmp(buffer, "ERROR the project already exists on server.")!=0){
            parseBuffer_create(buffer);
        }
        /*disconnect server at the end!*/
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "destroy") == 0))
    {
        sockfd = read_configure_and_connect();
        write_to_server(sockfd, argv[1], argv[2], argv[2]);
        char *buffer = read_from_server(sockfd);
        /*disconnect server at the end!*/
        block_write(sockfd, "4:Done", 6);
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
                if (fileExists(filePath))
                {
                    if (strcmp(argv[1], "add") == 0)
                    {
                        add_file_to_record(argv[2], filePath, manifestFilePath);
                    }
                    else if (strcmp(argv[1], "remove") == 0)
                    {
                        remove_file_from_record(argv[2], filePath, manifestFilePath);
                    }
                }
                else
                {
                    printf("Fatal Error: %s does not exist\n", filePath);
                }
            }
            else
            {
                printf("Fatal Error: %s does not exist\n", manifestFilePath);
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
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "upgrade") == 0))
    {
        /* Need to handle update error : project does not exist on server */
        sockfd = read_configure_and_connect();
        upgrade(argv[2], sockfd);
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "commit") == 0))
    {
        sockfd = read_configure_and_connect();
        write_to_server(sockfd, "manifest", argv[2], argv[2]);
        char* buffer = read_from_server(sockfd);
        write_commit_file(sockfd, argv[2], buffer);
    }
    else if (argc == 3 && (strcmp(argv[1], "push") == 0))
    {
        /*Get the data from the commit file*/
        sockfd = read_configure_and_connect();
        char *commit_file = (char *)malloc(strlen(argv[2]) + strlen("/.Commit"));
        commit_file[0] = '\0';
        strcat(commit_file, argv[2]);
        strcat(commit_file, "/.Commit");
        char *commitfile_content = read_file(commit_file);

        /*Get the data from the manifest file*/
        char *manifest_file = (char *)malloc(strlen(argv[2]) + strlen("/.Manifest"));
        manifest_file[0] = '\0';
        strcat(manifest_file, argv[2]);
        strcat(manifest_file, "/.Manifest");
        char *manifest_file_content = read_file(manifest_file);

        /*attach everything*/
        char *sec_cmd = (char *)malloc(strlen(argv[2]) + 1 + digits(strlen(commitfile_content)) + 1 + strlen(commitfile_content) + 1 + digits(strlen(manifest_file_content)) + 1 + strlen(manifest_file_content));
        sec_cmd[0] = '\0';
        strcat(sec_cmd, argv[2]);
        strcat(sec_cmd, ":");
        strcat(sec_cmd, to_Str(strlen(commitfile_content)));
        strcat(sec_cmd, ":");
        strcat(sec_cmd, commitfile_content);
        strcat(sec_cmd, ":");
        strcat(sec_cmd, to_Str(strlen(manifest_file_content)));
        strcat(sec_cmd, ":");
        strcat(sec_cmd, manifest_file_content);

        /*write to the server*/
        write_to_server(sockfd, argv[1], sec_cmd, argv[2]);
        while(1){
            char* buffer = read_from_server(sockfd);
            if(strstr(buffer, "sendfile")!=NULL){
                fetchFile(buffer, sockfd);
            } else {
                break;
            }
        } 
        remove_commit_file(sockfd, argv[2]);

        /*sync the server and client manifests*/
        write_to_server(sockfd, "manifest", argv[2], argv[2]);
        char* buffer = read_from_server(sockfd);
        syncManifests(argv[2], buffer);

    }
    else if (argc == 4 && (strcmp(argv[1], "rollback") == 0))
    {
        sockfd = read_configure_and_connect();
        char* sec_cmd = (char*)malloc(strlen(argv[2])+1+digits(strlen(argv[3]))+1+strlen(argv[3]));
        strcat(sec_cmd, argv[2]); //project name
        strcat(sec_cmd, ":");
        strcat(sec_cmd, to_Str(strlen(argv[3])));
        strcat(sec_cmd, ":");
        strcat(sec_cmd, argv[3]); //version number
        write_to_server(sockfd, "rollback", sec_cmd, argv[2]);
        char* buffer = read_from_server(sockfd);
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "currentversion") == 0))
    {
        sockfd = read_configure_and_connect();
        write_to_server(sockfd, "currentversion", argv[2], argv[2]);
        char* buffer = read_from_server(sockfd);
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else if (argc == 3 && (strcmp(argv[1], "history") == 0))
    {
        sockfd = read_configure_and_connect();
        get_history_file(argv[2], sockfd);
        block_write(sockfd, "4:Done", 6);
        printf("Client Disconnecting\n");
        close(sockfd);
    }
    else
    {
        printf("Fatal Error: Invalid Arguments\n");
    }
    return 0;
}
