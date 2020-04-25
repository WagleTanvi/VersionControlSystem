#ifndef RECORD_H
#define RECORD_H

/* Returns number of lines in file */
int number_of_lines(char *fileData);

/* Parses one line of the Record and addes to stuct */
void add_to_struct(char* line, Record** record_arr, int recordCount);

/* Returns an array of records */
Record** create_record_struct(char* fileData);

/* Returns size of record array which is stored in the first position of the array hash value */
int getRecordStructSize(Record** record_arr);

/* Look in the records for a particular file name formatted as project/filepath */
Boolean search_Record(Record** record_arr, char* targetFile);

/* Returns the hash in the records */
char* search_record_hash(Record** record_arr, char* targetFile);

/* Formats one record */
char* printRecord(Record* record);

/* Free Record Array */
void freeRecord(Record** record_arr);

#endif