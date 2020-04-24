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
#include "record.h"

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
void add_to_struct(char* line, Record** record_arr, int recordCount){
    int start = 0;
    int pos = 0;
    int count = 0;
    Record* record = (Record*) malloc(sizeof(Record));
    while (pos < strlen(line)){
        if (line[pos] == ' ' || line[pos] == '\n'){
            int len = pos - start;
            char *temp = (char *)malloc(sizeof(char) * len + 1);
            temp[0] = '\0';
            strncpy(temp, &line[start], len);
            temp[len] = '\0';
            switch (count){
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
            start = pos+1;
        }
        pos++;
    }
    record_arr[recordCount] = record;
}

/* Returns an array of records */
Record** create_record_struct(char* fileData){
    int start = 0;
    Boolean version = false;
    int pos = 0;
    int numberOfRecords = number_of_lines(fileData);
    Record** record_arr = (Record**) malloc(sizeof(Record*)*numberOfRecords);
    int recordCount = 0;
    while (pos < strlen(fileData))
    {
        if (fileData[pos] == '\n')
        {
            int len = pos - start;
            char *temp = (char *)malloc(sizeof(char) * len + 2);
            temp[0] = '\0';
            strncpy(temp, &fileData[start], len+1);
            temp[len+1] = '\0';
            if (version){ // if version number has already been seen
                add_to_struct(temp, record_arr, recordCount);
                start = pos+1;
                recordCount++;
                free(temp);
            }
            else{
                Record* record = (Record*) malloc(sizeof(Record));
                char* rec_count = (char*) malloc(sizeof(char)*50);
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
int getRecordStructSize(Record** record_arr){
    return atoi(record_arr[0]->hash);
}
/* Look in the records for a particular file name formatted as project/filepath */
Boolean search_Record(Record** record_arr, char* targetFile){
    int x = 1;
    int size = getRecordStructSize(record_arr);
    while ( x < size){
        if (strcmp(record_arr[x]->file, targetFile) == 0){
            return true;
        }
        x++;
    }
    return false;
}

char* search_record_hash(Record** record_arr, char* targetFile){
    int x = 1;
    int size = getRecordStructSize(record_arr);
    while ( x < size){
        if (strcmp(record_arr[x]->file, targetFile) == 0){
            return record_arr[x]->hash;
        }
        x++;
    }
    return NULL;
}