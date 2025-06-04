#ifndef _COMMANDS_H_
#define _COMMANDS_H_
int numberOfBuiltInCommands();
int CD(char **token, char **args);
int PWD(char **token, char **args);
int EXIT();
int LAUNCH(char **, char **);
int executeCommandFromTerminal(char **, char **);
char *traversing(char *, char *);
#endif