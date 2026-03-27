#ifndef BUILTINS_H
#define BUILTINS_H
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
extern char *builtin_str[];
extern int (*builtin_func[])(char **);
int lsh_num_builtins(void);
#endif