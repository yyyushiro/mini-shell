#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Function Declarations for builtin shell commands:
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
List of builtin commands, followed by their corresponding functions.
Since this is global, it can be accessed from anywhere.
*/ 
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

/*
List of function pointers, which prevents using if statements multiple times.
The parameter must be aligned, because we call one of the functions like (*builtin_func[i])(args);
*/
int (*builtin_func[]) (char**) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

/**
 * @brief This function gets the number of built-in functions.
 * 
 * @return int the number of built-in functions.
 */
int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/**
 * @brief This function is the implementation of the command 'cd'.
 * 
 * @param args the command 'cd' and a path.
 * @return int status code.
 */
int lsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        // use chdir function to change the current directory.
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

/**
 * @brief This function is the implementation of the command 'help'.
 * 
 * @param args arguments.
 * @return int status code.
 */
int lsh_help(char **args)
{   
    (void)args;
    printf("Yushiro Murakami's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

/**
 * @brief This function is the implementation of the command 'exit'.
 * 
 * @param args arguments.
 * @return int status code.
 */
int lsh_exit(char **args)
{   
    (void)args;
    // By setting the status code 0, the lsh_loop function can escape from while loop.
    return 0;
}