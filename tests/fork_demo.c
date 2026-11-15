/* Educational example: fork(), execvp(), and waitpid(). */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        char *args[] = { "ls", "-l", NULL };
        puts("Child: executing ls -l");
        fflush(stdout);
        execvp(args[0], args);
        perror("execvp");
        _exit(127);
    }

    printf("Parent: waiting for child %ld\n", (long)pid);
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("Parent: child exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Parent: child was terminated by signal %d\n", WTERMSIG(status));
    }

    return 0;
}
