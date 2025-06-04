#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int i;
    int cat_id, sort_id, child_id, wait_status, pipe_fd[2];
    char *child_name;
    cat_id = fork();

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    if (cat_id == 0) {
        char **cat_args = malloc(sizeof(char *) * (argc + 1));

        cat_args[0] = "cat";
        for (j = 1; j < argc; j++) {
            cat_args[j] = argv[j];
        }
        
        cat_args[argc] = NULL;
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execvp("cat", cat_args);
        perror("cat");
        return EXIT_FAILURE;
    }

    sort_id = fork();
    if (sort_id == 0) {
        dup2(pipe_fd[0], STDIN_FILENO);

        close(pipe_fd[0]);
        close(pipe_fd[1]);

        execlp("sort", "sort", NULL);

        perror("sort");
        return EXIT_FAILURE;
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    for (i = 0; i < 2; i++) {
        child_id = wait(&wait_status);
        child_name = (child_id == cat_id) ? "cat" : "sort";
        if (WIFEXITED(wait_status)) {
            printf("%s The program has exited with status %d\n", child_name, WEXITSTATUS(wait_status));
        } else {
            printf("%s The program has exited abnormally \n", child_name);
        }
    }

    return EXIT_SUCCESS;
}