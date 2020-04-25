#include "helper.h"
#include "record.h"

/* Parses one line of the Record and addes to stuct */
void add_to_struct(char* line, Record** record_arr, int recordCount){
    int start = 0;
    int pos = 0;
    int count = 0;
    Record* record = (Record*) malloc(sizeof(Record));
    while (pos < strlen(line)){
        if (line[pos] == ' ' || line[pos] == '\n'){
            int len = pos - start;
            char* temp = (char*) malloc(sizeof(char)*len+1);
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
Record** create_record_struct(char* fileData, int size, char flag){
    int start = 0;
    Boolean version = false;
    if(flag == 'C') version = true;
    int pos = 0;
    int numberOfRecords = number_of_lines(fileData)+size;
    Record** record_arr = (Record**) malloc(sizeof(Record*)*numberOfRecords);
    int recordCount = 0;
    while (pos < strlen(fileData)){
        if (fileData[pos] == '\n'){
            int len = pos - start;
            char* temp = (char*) malloc(sizeof(char)*len+2);
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
                start = pos+1;
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

/* Look in the record array for a particular file name formatted as project/filepath */
Record* search_record(Record** record_arr, char* targetFile){
    int x = 1;
    int size = getRecordStructSize(record_arr);
    while (x < size){
        if (strcmp(record_arr[x]->file, targetFile) == 0){
            return record_arr[x];
        }
        x++;
    }
    return NULL;
}

/* Formats one record */
char* printRecord(Record* record){
    int len;
    if (record == NULL){
        return NULL;
    }
    else{
        int len = strlen(record->version)+strlen(record->file)+strlen(record->hash) + 1 + 3;
        char* line = (char*) malloc(sizeof(char)*len);
        line[0] = '\0';
        strcat(line, record->version);
        strcat(line, " ");
        strcat(line, " ");
        strcat(line,record->file );
        strcat(line, " ");
        strcat(line,record->hash );
        return line;
    }
}

/* Free Record Array */
void freeRecord(Record** record_arr){
    int size = getRecordStructSize(record_arr);
    free(record_arr[0]->version);
    free(record_arr[0]->hash);
    free(record_arr[0]);
    int x = 1;
    while (x < size){
        free(record_arr[x]->version);
        free(record_arr[x]->file);
        free(record_arr[x]->hash);
        free(record_arr[x]);
        x++;
    }
    free(record_arr);
}