#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>  // for open flags
#include <sys/stat.h>  // for file permissions
#include <sys/types.h> // for data types
#include <unistd.h>  // for read/write/close functions

#define MAX_CMD_LEN 1024
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

void process_input(char* input) 
{
    char* args[MAX_CMD_LEN];
    char* arg;
    int arg_count = 0;

    arg = strtok(input, " ");
    while (arg != NULL) {
        args[arg_count++] = arg;
        arg = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        return;
    }

    int pipe_index = -1;
    for (int i=0; i<arg_count; i++) {
        if(strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }
    if (pipe_index != -1) {
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            printf("Error: Could not create pipe\n");
            return;
        }
    
        args[pipe_index] = NULL;
        pid_t pid1 = fork();
        if (pid1 == -1) {
            printf("Error: Could not fork process\n");
            return;
        } else if (pid1 == 0) {
            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
            execvp(args[0], args);
            printf("Error: Command not found\n");
            exit(1);
        }

        pid_t pid2 = fork();
        if(pid2 == -1) {
            printf("Error: Could not fork process\n");
            return;
        } else if(pid2 == 0) {
            close(pipe_fd[1]);
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);
            execvp(args[pipe_index + 1], &args[pipe_index + 1]);
            printf("Error: Command not found\n");
            exit(1);
        }   
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        int status;
        waitpid(pid1, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                printf("Error: Command exited with status %d\n", exit_status);
            }
        }
        waitpid(pid2, &status, 0);
        if(WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                printf("Error: Command exited with status %d\n", exit_status);
            }
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (arg_count > 1) {
            if (chdir(args[1]) != 0) {
                printf("Error: Could not change directory\n");
            }
        } else {
            printf("Error: Missing argument for cd command\n");
        }
    } else if (strcmp(args[0], "ls") == 0) {
        DIR* dir;
        struct dirent* dp;

        if (arg_count > 2) {
            printf("Error: Too many arguments for ls command\n");
            return;
        }

        if (arg_count == 1) {
            dir = opendir(".");
        } else {
            dir = opendir(args[1]);
        }

        if (dir == NULL) {
            printf("Error: Could not open directory %s\n", args[1]);
            return;
        }

        while ((dp = readdir(dir)) != NULL) {
            printf("%s\n", dp->d_name);
        }

        closedir(dir);
    } else {
        pid_t pid = fork();
        if (pid == -1) {
            printf("Error: Could not fork process\n");
            return;
        } else if (pid == 0) {
            // Child process
            execvp(args[0], args);
            printf("Error: Command not found\n");
            exit(1);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status != 0) {
                    printf("Error: Command exited with status %d\n", exit_status);
                }
            }
        }
    }
}

void interactive_mode() {
    printf("Welcome to my shell in Interactive Mode!\n");
    char cmd[MAX_CMD_LEN];
    char cwd[MAX_CMD_LEN];
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf(COLOR_CYAN "%s" COLOR_RESET "$ ", cwd);
        } else {
            printf(COLOR_CYAN "Unknown" COLOR_RESET "$ ");
        }
        fflush(stdout);
        if(fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }
        cmd[strcspn(cmd, "\n")] = '\0'; 
        process_input(cmd);
    }
}

void batch_mode(char* filename) {
    printf("Welcome to my shell in Batch Mode!\n");
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error: Unable to open file %s\n", filename);
        exit(1);
    }

    char cmd[MAX_CMD_LEN];
    while (fgets(cmd, MAX_CMD_LEN, file) != NULL) {
        cmd[strcspn(cmd, "\n")] = '\0'; 
        process_input(cmd);
    }
    fclose(file);
}    

int main(int argc, char* argv[]) {
    
    if(argc == 1) {
        interactive_mode();
    } else if (argc == 2) {
        batch_mode(argv[1]);
    } else {
        printf("Error: Too many arguments\n");
        exit(1);
    }
    return 0;

    
    if (argc  > 2) {
        fprintf(stderr, "usage: %s [script]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int fd;
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if(fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    int fd1, fd2, n;
    char buf[1];
    fd1 = open("input.txt", O_RDONLY);
    fd2 = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while ((n = read(fd1, buf, 1)) > 0) {
        write(fd2, buf, 1);
    }
    close(fd1);
    close(fd2);

    while(1) {
        char command[1024];
        printf(">> ");
        fflush(stdout);
        ssize_t num_read = read(STDIN_FILENO, command, sizeof(command));
        if(num_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(num_read == 0) {
            break;
        }
        if(command[num_read-1] == '\n') {
            command[num_read-1] = '\0';
        }
        system(command);
    } 
    return 0;
}