#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>  /* for wait */
#include <errno.h>
int main()
{
    rmdir("../client/proj0");
    chdir(getenv("HOME"));
    chdir("Asst3/server");
    rmdir("proj0");
    /*Spawn a child to run the program.*/
    pid_t pid = fork();
    if (pid == 0)
    { /* child process */
        static char *argv[] = {"WTFServer", "9000", NULL};
        chdir(getenv("HOME"));
        if (execvp("Asst3/server/WTFServer", argv) == -1)
        {
            printf("hey");
            printf("failed connection %s\n", strerror(errno));
            //printf("%s", strerror(errno));
            exit(127); /* only if execv fails */
        }
    }
    else
    {
        chdir(getenv("HOME"));
        chdir("Asst3/client");
        printf("Running: ./WTF create proj0\n");
        system("ls");
        system("./WTF add proj0 file1.txt");
        printf("Running: ./WTF checkout proj0\n");
        system("./WTF checkout proj0");
        // system("./WTF checkout proj0");
        // system("./WTF current version");
        // system("./WTF update proj0");
        // system("./WTF upgrade proj0");
        // system("./WTF commit proj0");
        // system("./WTF push proj0");
        // system("./WTF rollback proj0 2");
        // system("./WTF history proj0");
        // system("./WTF destroy proj0");
        // system("./WTF destroy proj0");
        waitpid(pid, 0, 0);
    }
    // else
    // { /* pid!=0; parent process */
    //     pid_t pidF = fork();
    //     if (pidF == 0)
    //     {
    //         static char *argv[] = {"WTF", "configure", "java.cs.rutgers.edu", "9000", NULL};
    //         chdir(getenv("HOME"));
    //         chdir("./Asst3/client");
    //         if (execvp("WTF", argv) == -1)
    //         {
    //             printf("hey");
    //             printf("failed connection %s\n", strerror(errno));
    //             //printf("%s", strerror(errno));
    //             exit(127); /* only if execv fails */
    //         }
    //     }
    //     printf("hey");
    //     waitpid(pidF, 0, 0);
    //     waitpid(pid, 0, 0); /* wait for child to exit */
    // }
    //waitpid(pid, 0, 0);
    return 0;
}