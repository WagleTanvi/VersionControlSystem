#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
int main()
{
    int fd = open("tests", O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (fd < 0)
    {
        return 0;
    }

    write(fd, "cd server", 9);
    write(fd, "rm -rf proj0", 12);
    write(fd, "./WTFServer 9000 &", 18);
    write(fd, "cd ../client", 12);
    write(fd, "rm -rf proj0", 12);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running Create proj0'", 27);
    write(fd, "./WTF create proj0", 18);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running checkout proj0'", 29);
    write(fd, "./WTF checkout proj0", 20);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running currentversion proj0'", 35);
    write(fd, "./WTF currentversion proj0", 26);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running update proj0'", 27);
    write(fd, "./WTF update proj0", 18);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running upgrade proj0'", 28);
    write(fd, "./WTF upgrade proj0", 19);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running commit proj0'", 27);
    write(fd, "./WTF commit proj0", 18);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running push proj0'", 25);
    write(fd, "./WTF push proj0", 16);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running rollback proj0'", 29);
    write(fd, "./WTF rollback proj0 2", 22);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running destroy proj0'", 28);
    write(fd, "./WTF destroy proj0", 19);
    write(fd, "echo '=============================================='", 53);
    write(fd, "echo 'Running destroy proj0'", 28);
    write(fd, "./WTF destroy proj0", 19);
    write(fd, "pkill WTFServer", 15);

    close(fd);
    // pid_t pid = fork();
    // if (pid == 0)
    // { /* child process */
    //     static char *argv[] = {"bash", "Asst3/tests", NULL};
    //     chdir(getenv("HOME"));
    //     if (execvp("/bin/bash", argv) == -1)
    //     {
    //         //printf("hey");
    //         printf("failed connection %s\n", strerror(errno));
    //         //printf("%s", strerror(errno));
    //         exit(127); /* only if execv fails */
    //     }
    // }
    // else
    // {
    //     waitpid(pid, 0, 0);
    // }
}