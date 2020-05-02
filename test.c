#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>  /* for wait */
#include <errno.h>
int main()
{
    /*Spawn a child to run the program.*/
    pid_t pid = fork();
    if (pid == 0)
    { /* child process */
        static char *argv[] = {"9000", NULL};
        chdir("server");
        execvp("./WTF", argv);
        //printf("%s", strerror(errno));
        exit(127); /* only if execv fails */
    }
    else
    {                       /* pid!=0; parent process */
        waitpid(pid, 0, 0); /* wait for child to exit */
    }
    printf("HEY: %s");
    return 0;
}