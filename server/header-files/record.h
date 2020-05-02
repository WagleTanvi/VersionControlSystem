#ifndef RECORD_H
#define RECORD_H

/* Parses one line of the Record and addes to stuct */
void add_to_struct(char* line, Record** record_arr, int recordCount);

/* Returns an array of records */
Record** create_record_struct(char* fileData, int size, char flag);

/* Returns size of record array which is stored in the first position of the array hash value */
int getRecordStructSize(Record** record_arr);

/* Look in the record array for a particular file name formatted as project/filepath */
Record* search_record(Record** record_arr, char* targetFile);

/* Formats one record */
char* printRecord(Record* record);

/* Free Record Array */
void freeRecord(Record **record_arr, char flag, int size);

#endif