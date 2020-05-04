#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
int test1(int fd)
{
    write(fd, "rm -rf proj0\n", 13);
    write(fd, "echo 'RUNNING SEQUENCE 1'\n", 26);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "./WTF create proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running Create proj0'\n", 28);
    write(fd, "echo 'Expected output: Success'\n", 32);
    write(fd, "./WTF create proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running checkout proj0'\n", 30);
    write(fd, "echo 'Expected output: Proj already exists'\n", 44);
    write(fd, "./WTF checkout proj0\n", 21);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running currentversion proj0'\n", 36);
    write(fd, "echo 'Expected output: 0'\n", 26);
    write(fd, "./WTF currentversion proj0\n", 27);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running update proj0'\n", 28);
    write(fd, "echo 'Expected output: No updates'\n", 35);
    write(fd, "./WTF update proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running upgrade proj0'\n", 29);
    write(fd, "echo 'Expected output: Up to date'\n", 35);
    write(fd, "./WTF upgrade proj0\n", 20);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running commit proj0'\n", 28);
    write(fd, "echo 'Expected output: No commit changes'\n", 42);
    write(fd, "./WTF commit proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running push proj0'\n", 26);
    write(fd, "echo 'Expected output: No commits for push'\n", 44);
    write(fd, "./WTF push proj0\n", 17);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running rollback proj0'\n", 30);
    write(fd, "echo 'Expected output: No previous version'\n", 44);
    write(fd, "./WTF rollback proj0 2\n", 23);
    // write(fd, "echo '=============================================='\n", 54);
    // write(fd, "echo 'Running destroy proj0'\n", 29);
    // write(fd, "echo 'Expected output: Success'\n", 32);
    // write(fd, "./WTF destroy proj0\n", 20);
    // write(fd, "echo '=============================================='\n", 54);
    // write(fd, "echo 'Running destroy proj0'\n", 29);
    // write(fd, "echo 'Expected output: Error'\n", 31);
    // write(fd, "./WTF destroy proj0\n", 20);
}
int test2(int fd)
{
    write(fd, "echo 'RUNNING SEQUENCE 2'\n", 26);
    write(fd, "rm -rf proj0\n", 13);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running update proj0'\n", 28);
    write(fd, "echo 'Expected output: proj does not exist'\n", 44);
    write(fd, "./WTF update proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running upgrade proj0'\n", 29);
    write(fd, "echo 'Expected output: proj does not exist'\n", 44);
    write(fd, "./WTF upgrade proj0\n", 20);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running Create proj0'\n", 28);
    write(fd, "echo 'Expected output: Success'\n", 32);
    write(fd, "./WTF create proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running upgrade proj0'\n", 29);
    write(fd, "echo 'Expected output: .Update file does not exist'\n", 52);
    write(fd, "./WTF upgrade proj0\n", 20);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "touch proj0/file.txt\n", 21);
    write(fd, "echo test > proj0/file.txt\n", 27);
    write(fd, "echo 'Running add proj0 file.txt'\n", 34);
    write(fd, "echo 'Expected output: Success'\n", 32);
    write(fd, "./WTF add proj0 file.txt\n", 25);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running commit proj0'\n", 28);
    write(fd, "echo 'Expected output: Add file.txt'\n", 37);
    write(fd, "./WTF commit proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running push proj0'\n", 26);
    write(fd, "echo 'Expected output: Success'\n", 32);
    write(fd, "./WTF push proj0\n", 17);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Contents of file1.txt on server should be: test'\n", 55);
    write(fd, "cat ../server/proj0/file.txt\n", 29);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running update proj0'\n", 28);
    write(fd, "echo 'Expected output: No updates'\n", 35);
    write(fd, "./WTF update proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running upgrade proj0'\n", 29);
    write(fd, "echo 'Expected output: Up to date'\n", 35);
    write(fd, "./WTF upgrade proj0\n", 20);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running rollback proj0 0'\n", 32);
    write(fd, "echo 'Expected output: Success'\n", 32);
    write(fd, "./WTF rollback proj0 0\n", 23);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'file1.txt should not exist'\n", 34);
    write(fd, "cat ../server/proj0/file.txt\n", 29);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running update proj0'\n", 28);
    write(fd, "echo 'Expected output: Delete file.txt'\n", 40);
    write(fd, "./WTF update proj0\n", 19);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Running upgrade proj0'\n", 29);
    write(fd, "echo 'Expected output: success'\n", 32);
    write(fd, "./WTF upgrade proj0\n", 20);
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'file1.txt should not exist in manifest'\n", 46);
    write(fd, "cat proj0/.Manifest\n", 20);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Enter a host, a port and sequence number 1-2\n");
        return 0;
    }
    int fd = open("tests", O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (fd < 0)
    {
        return 0;
    }
    write(fd, "cd server\n", 10);
    write(fd, "rm -rf proj0\n", 13);
    write(fd, "./WTFServer ", 12);
    write(fd, argv[2], strlen(argv[2]));
    write(fd, " &\n", 3);
    write(fd, "cd ../client\n", 13);
    write(fd, "echo 'Running Configure proj0'\n", 31);
    write(fd, "./WTF configure ", 16);
    write(fd, argv[1], strlen(argv[1]));
    write(fd, " ", 1);
    write(fd, argv[2], strlen(argv[2]));
    write(fd, "\n", 1);
    if (strcmp(argv[3], "1") == 0)
    {
        test1(fd);
    }
    else if (strcmp(argv[3], "2") == 0)
    {
        test2(fd);
    }
    else
    {
        printf("Only sequence numbers are 1 and 2\n");
        unlink("tests");
        return 0;
    }
    write(fd, "echo '=============================================='\n", 54);
    write(fd, "echo 'Killing Server Proccess'\n", 31);
    write(fd, "pkill WTFServer\n", 16);
    write(fd, "echo 'Tests Completed'\n", 23);

    close(fd);
    pid_t pid = fork();
    if (pid == 0)
    { /* child process */
        char *home = getenv("HOME");
        char *path = (char *)malloc(sizeof(char) + strlen(home) + 12 + 1);
        strcpy(path, home);
        strcat(path, "/Asst3/tests");
        printf("PATH %s", path);
        static char *argv[3];
        argv[0] = "bash";
        argv[1] = path;
        argv[2] = home;
        if (execvp("/bin/bash", argv) == -1)
        {
            //printf("hey");
            printf("failed connection %s\n", strerror(errno));
            //printf("%s\n", strerror(errno));
            exit(127); /* only if execv fails */
        }
    }
    else
    {
        waitpid(pid, 0, 0);
    }
    //unlink("tests");
}