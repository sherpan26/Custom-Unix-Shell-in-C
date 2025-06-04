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

static void wildcard(cmd_t *cmd, char *path) {
    char *asterisk = strchr(path, '*');
    if (asterisk == NULL) {
        array_add(cmd->args, copy_string(path));
        return;
    }

    char *old_path = copy_string(path);
    *asterisk = '\0';

    char *prefix = path;
    char *base = strrchr(path, '/');
    if (base == NULL) {
        base = ".";
    } else {
        *base = '\0';
        prefix = base + 1;
        base = path;
    }

    int prefix_len = strlen(prefix);
    char *suffix = asterisk + 1;
    int suffix_len = strlen(suffix);
    /* printf("base: %s, prefix: %s, suffix: %s\n", base, prefix, suffix); */

    DIR *dir = opendir(base);
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        int name_len = strlen(entry->d_name);
        if (prefix_len + suffix_len <= name_len &&
            strncmp(entry->d_name, prefix, prefix_len) == 0 &&
            strcmp(entry->d_name + name_len - suffix_len, suffix) == 0) {
            char *new_path = malloc(strlen(base) + 1 + name_len + 1);
            if (strcmp(base, ".") == 0) {
                sprintf(new_path, "%s", entry->d_name);
            } else {
                sprintf(new_path, "%s/%s", base, entry->d_name);
            }
            count++;
            /* printf("new_path: %s\n", new_path); */
            array_add(cmd->args, new_path);
        }
    }
    if (count == 0) {
        array_add(cmd->args, old_path);
    } else {
        free(old_path);
    }
    closedir(dir);
}

static bool cmd_parse(char *line, cmd_t *cmd) {
    char *token = NULL;
    while ((token = next_token(&line, DELIM)) != NULL) {
        if (strcmp(token, "<") == 0) {
            if (cmd->in != NULL) {
                return false;
            }
            char *next = next_token(&line, DELIM);
            if (next == NULL) {
                return false;
            }
            cmd->in = copy_string(next);
        } else if (strcmp(token, ">") == 0) {
            if (cmd->out != NULL) {
                return false;
            }
            char *next = next_token(&line, DELIM);
            if (next == NULL) {
                return false;
            }
            cmd->out = copy_string(next);
        } else {
            if (token[0] == '~') {
                token = extension_home_dir(token);
                wildcard(cmd, token);
                free(token);
            } else {
                wildcard(cmd, token);
            }
        }
    }

    if (array_size(cmd->args) == 0) {
        return false;
    }

    array_add(cmd->args, NULL);
    return true;
}

pipeline_t *pipeline_parse(char *line) {
    pipeline_t *pipeline = malloc(sizeof(pipeline_t));
    pipeline->cmds = array_new(5);

    char *token = NULL;
    while ((token = next_token(&line, "|")) != NULL) {
        cmd_t *cmd = cmd_new();
        if (!cmd_parse(token, cmd)) {
            cmd_free(cmd);
            pipeline_free(pipeline);
            return NULL;
        }
        array_add(pipeline->cmds, cmd);
    }

    for (int i = 0; i < array_size(pipeline->cmds); i++) {
        cmd_t *cmd = array_get(pipeline->cmds, i);
        if (cmd->in != NULL && i != 0) { // only the first command can have input
            pipeline_free(pipeline);
            return NULL;
        }
        if (cmd->out != NULL && i != array_size(pipeline->cmds) - 1) { // only the last command can have output
            pipeline_free(pipeline);
            return NULL;
        }
    }

    return pipeline;
}

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
                printf("!mysh >");
                return;
            }
        }
        waitpid(pid2, &status, 0);
        if(WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                printf("Error: Command exited with status %d\n", exit_status);
                printf("!mysh >");
                fflush(stdout);
                return;
            }
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if(args[1] == NULL) {
            chdir(getenv("HOME"));
        }
        else if(chdir(args[1]) != 0) {
            printf("Error: No such directory\n");
        } else {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s", cwd);
                printf("$ exit\n");
                printf("mysh: exiting");
                printf("\n$");
                fflush(stdout);
            }
            else {
                perror("getcwd() error");
            }
            exit(0);
        }
        if (arg_count > 1) {
            char* cd_cmd = malloc(strlen(args[1]) + 4);
            sprintf(cd_cmd, "cd %s", args[1]);
            system(cd_cmd);
             free(cd_cmd);
            if (chdir(args[1]) != 0) {
                printf("cd: No such file or directory\n");
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
    } else if (chdir("../..") == 0) {
        char cwd[1024];
        if(getcwd(cwd, sizeof(MAX_CMD_LEN)) != NULL) {
            printf("Current working directory: %s\n", cwd);
        } else {
            printf("Error: Failed to change directory.\n");
        }
     
        
    
    }    else {
        pid_t pid = fork();
        if (pid == -1) {
            printf("Error: Could not fork process\n");
            return;
        } else if (pid == 0) {
            // Child process
            execvp(args[0], args);
            printf("Error: Command not found\n");
            exit(1);
        } else if (pid <0) {
            perror("Error");
        }
        else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status != 0) {
                    printf("Error: Command exited with status %d\n", exit_status);
                    printf("!mysh >");
                    fflush(stdout);
                    return;
                }
            }
        }
        printf("!");
    }
            printf("!");

}

void interactive_mode() {
    printf("Welcome to my shell in Interactive Mode!\n");
    char cmd[MAX_CMD_LEN];
    char cwd[MAX_CMD_LEN];
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf(COLOR_CYAN "mysh> " COLOR_RESET);    
        } else {
            printf(COLOR_CYAN "Unknown" COLOR_RESET "$ ");
        }
        fflush(stdout);
        if(fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }
        cmd[strcspn(cmd, "\n")] = '\0'; 
        if (strcmp(cmd, "exit") == 0) {
            printf("Exiting my shell\n");
            break;
        }
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
///////
int run(char* args[], int input, int output) {
    pid_t p = fork();
    if (p == -1) {
        perror("fork");
        return -1;
    }
    else if (p == 0) {
        if (input != STDIN_FILENO) {
            dup2(input, STDIN_FILENO);
            close(input);
        }

        if (output != STDOUT_FILENO) {
            dup2(output, STDOUT_FILENO);
            close(output);
        }


    }
}
///////
int main(int argc, char* argv[]) {
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set\n");
        exit(1);
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))==NULL) {
        perror("getcwd() error");
        exit(1);
    }
    printf("Current working directory: %s\n", cwd);

    if(argc == 1 || (argc == 2 && strncmp(argv[1], "~/", 2) == 0)) {
        char *target_dir = (argc == 1) ? home : strcat(home, argv[1] + 1);
        if(chdir(target_dir) == -1) {
            perror("chdir() error");
            exit(1);
        }
        printf("Changed working directory to: %s\n", target_dir);
    } else {
        if (chdir(argv[1]) == -1) {
            perror("chdir() error");
            exit(1);
        }
        printf("Changed working directory to: %s\n", argv[1]);
    }

    if(getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(1);
    }
    printf("New working directory: %s\n", cwd);
    
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