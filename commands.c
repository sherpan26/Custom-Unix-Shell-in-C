#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <wait.h>
#include <dirent.h>
#include "CommandName.h"
#include "words.h"

char *BuiltInCommands[] = {"exit", "cd", "pwd"};
char *PathName [] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin","/usr/bin", "/sbin", "/bin"};
int *BuiltInFunctions[] (char **, char **) = {&CD, &PWD, &EXIT };

int numberOfBuiltInCommands()
{
    return sizeof(BuiltInCommands)/sizeof(char *);
}

int numberOfRoutes() 
{
    return sizeof(Routes)/sizeof(char *);
    
    }

int CD(char **token, char **argv) {
    char *HOME =  getenv("HOME");
    if(argv[1] == NULL) {
        chdir(HOME);
        return 0;
    }
    if (chdir(args[1]) != 0) {
        perror("myShellCommand");
        return 1;
    }
    return 0;
}

int PWD(char **token, char **argv) {
    char array[256];
    int output = 0;
    int file;
    char *outputFile;

    for (int i = 0; token[i] != NULL; i++) {
        if (strcmp(token[i], ">") == 0) {
            output = 1;
            if (token[ i + 1] == NULL) {
                printf("Redirection Error");
                return 1;

            }
            outputFile = token[i+1];
            break;
        }
        if(strcmp(token[i], "|") == 0) {
            output = 1;
            if (token[i + 1] == NULL) {
                printf("Redirection Error");
                return 1;
            }
            outputFile = token[i + 1];
            break;
        }
    }
    if (getcwd(arrya, sizeof(array)) == NULL) {
        perror("myShellPrintWorkingDirectory");
        return 1;
    }
    else if(output) {
        file = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP);
        if (file == -1) {
            perror("myShell: ");
            return 1;
        }
        write(file, array, strlen(array));
    }
    else {
        printf("%s\n", array);
    }
    close(file);
    return 0;
}

int EXIT() {
    printf("myshell is exiting\n");
    exit(EXIT_SUCCESS);
    return 0;
}

int LAUNCH(char **token, char **args) {
    int pipes = 0;
    int output = 0;
    int input = 0;
    int PathName = 0;
    int stdinput = STDIN_FILENO;
    int stdoutput = STDOUT_FILENO;
    
    char *filein = NULL, *fileout = NULL;

    for (int i = 0; i < strlen(token[0]) + 1; i++) {
        if(token[0][i]== '/') {
            PathName = 1;
            break;
        }
    }

    for (int i = 0; token[i] != NULL; i++) {
        if (strcmp(token[i], ">") == 0) {
                output = 1;

                if (token[i + 1] == NULL) {
                    printf("Redirection error: no specified file\n");
                    return 1;
                }
                fileout = tokens[i+1];

        }
        
        else if (strcmp(tokens[i], "<") == 0) {
            input = 1;
            if (token[i+1] == NULL) {
                    printf("Redirection error: no specified file\n");
                    return 1;
            }
            filein = token[i+1];
            args = get_args(token);
            
            
        }
        if(strcmp(tokens[i],"|") == 0) {
            pipes++;
        }
    }
    if(input) {
        stdinput = open(filein, O_RDONLY);
        if(stdinput == -1) {
            printf("mysh: input open file has failed\n");
            return 1;
        }
    }
    if(output) {
        stdoutput = open(fileout, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP);
        if(stdoutput == -1) {
            perror("mysh: ");
            return 1;
        }
    }

    if(input) {
        stdinput = open(fileout, O_RDONLY);
        if(stdoutput == -1) {
            perror("mysh: ");
            return 1;
        }
    }
    if(pipes != 0) {
        int p[pipes*2];
        pid_t pid[pipes + 1];
        for( int i = 0; i < pipes; i++) {
            if(pipe(p + i * 2) == -1) {
                perror("pipe");
                exit(1);
            }
        }
        for (int i = 0; i < pipes; i++) {
            pid[i] = fork();
            if(pid[i] == -1) {
                perror("fork");
                exit(1);
            } else if (pid[i] == 0) {
                if(i = 0) {
                    close(p[0]);
                    dup2(p(1), STDIN_FILENO);
                    close(p[1]);
                    execvp(args[1], args);
                    perror("execvp");
                    exit(1);
                } else if (i == pipes) {
                    close(p[(i-1) *2]);
                    dup2(p[(i-1)*2 +1], STDOUT_FILENO);
                    close(p[(i-1)*2 + 1]);
                    execvp(args[i+1], args);
                    perror("execvp");
                    exit(1);
                } else {
                    close(p[(i-1)*2]);
                    dup2(p[(i-1)*2 + 1], STDOUT_FILENO);
                    close(p[(i-1)*2 + 1]);
                    close(p[i*2 + 1]);
                    dup2(p[i*2], STDIN_FILENO);
                    close(p[i*2]);
                    execvp(args[i+1], args);
                    perror("execlp");
                    exit(1);
                }
            }
        }
        for(int i = 0; i <= pipes; i++) {
            waitpid(pid[i],NULL,0);
        }
    } else {
        pid_t pid, waitpid;
        int s;
        char *pathname = NULL;
        if(!PathName) {
            for(int i = 0; i < numberOfRoutes(); i++) {
                pathname = traversing(Routes[i], args[0]);
                if(pathname != NULL) {
                    break;
                }
            }


        }
        
        if(pathname == NULL && !PathName) {
            perror("myShell");
            return 1;
            
        }
        pid = fork()
        if (pid < 0) {
            perror("myShell: ");
            return 1;
        } else if (pid == 0) {
            if (output) {
                dup2(outputFile, STDOUT_FILENO);
            }

            if(input) {
                dup2(stdinput, STDIN_FILENO);
            }
            if(PathName) {
                execv(args[0], args);
                printf("exec has been failed\n");
                return 1;
            } else {
                execv(PathName, args);
                printf("exec has been failed\n");
                return 1;
            }
        } else {
            do {
                wair(NULL);
            } while (!WIFEXITED(s) && !WIFSIGNALED(s));
        }
        if(input) {
            close(stdinput);
        }
        if(output) {
            close(stdoutput);
        }
        free(pathname);

    }
    return 0;
} 

int executeCommandFromTerminal(char **t, char **arg) {
    int num;
    if(arg[0] == NULL) 
    {
        return 1
    }
    for (int j = 0; j < numberOfBuiltInCommands(); i++) {
        if(strcmp(arg[0], BuiltInCommands[i]) == 0) {
            return (*BuiltInFunctions[i])(t, arg);
        }
    }
    num = LAUNCH(t,arg);
    return num;
}
char *traversing(char *n, char *final) {
    struct dirent *d;
    long offset;
    int ste;
    int len = strlen(n);
    char *pathname;
    DIR *d = opendir(n);
    if(!d) {
        perror(n);
        return NULL;
    }

    ste = strlen(final);
    pathname = malloc(len + ste + 2);
    memcpy(pathname, n, len);
    pathname[len] = '/';
    memcpy(pathname + len + 1, final, ste);
    pathname[len + 1 + ste] = '\0';

    if(access(pathname, F_OK) ==0) {
        return pathname
    }
    while(d = readdir(d)) {
        if (d -> d_type == DT_DIR && de->d_name[0] != '.') {
            ste = strlen(de->d_name);
            pathname = realloc(pathname, len + ste +2);
            memcpy(pathname, len);
            pathname[len] = '/';
            memcpy(pathname, len + 1, de->d_name, ste);
            pathname[len +1+ste] = '\0';
            offset = telldir(d);
            closedir(d);
            traversing(pathname, final);
            free(pathname);
            d = opendir(d_name);
            seekdir(s, offset);

        }
    }
    closedir(d);
    return NULL;
}