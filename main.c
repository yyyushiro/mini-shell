#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "builtins.h"
#include <limits.h>



#define LSH_RL_BUFSIZE 1024
/**
 * @brief Gets characters from input and turn them into a line.
 * 
 * @return char* the whole string.
 */
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = (char *)malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        /*
        getchar() reads a character from stdin. It waits a character from stdin until new characters come.
        This is the reason we can implement bachslash escaping.
        */
        c = getchar();

        // If we hit EOF, replace it with a null character and return.
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else if (c == '\\') {
            c = getchar();
            buffer[position] = ' ';
            printf(">");
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocatoin error \n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
 * @brief This function separates a string into tokens.
 * 
 * @param line the given string to be separated.
 * @return char** the array of tokens.
 */
char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*)); // make an array of pointers of string.
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocatoiin error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM); // we may replace it with strtok_r
    while (token != NULL) {
        tokens[position] = token;
        position++;
        
        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
        tokens[position] = NULL;
    }
    return tokens;
}

/**
 * @brief This function calls fork() and execpv() to make a child process and run the specified function.
 * 
 * @param args arguments including function name and other parameters.
 * @return int status code.
 */
int lsh_launch(char **args) 
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            // file is the name of program
            // this line is reached only when the program failed.
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
        return -1;
    } else {
        // Parent process
        do {
            /*
            waitpid is a function that waits the child process ends.
            The parent process stops until the child process ends.
            After it finished, status gets an int representing how the child process ended, used for if it breaks the while loop.
            WUNTRACED allows waitpid to detect when the child process stops.
            wpid gets child's pid when waitpid succeeds, -1 when fails.
            */
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1) {
                perror("lsh");
                /*
                Since this is called from lsh_execute, if it returns 0, then status becomes 0 and loop stops in lsh_loop.
                We are going to return -1, in this case.
                */
                return -1; 
                        
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return 1;
    }
}

/**
 * @brief This function takes arguments and decide if it calls a built-in function or make another process.
 * 
 * @param args arguments including function name and other parameters.
 * @return int status code.
 */
int lsh_execute(char **args)
{
    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }
    // If the given function name is built-in, then just calls it.
    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i]) (args);
        } 
    }
    // If not built-in, then use fork() and execvp() to start new process.
    return lsh_launch(args);
}

/**
 * @brief This function ttakes user's input and execute command according to 
 * 
 */
void lsh_loop(void)
{
    char *line;
    char **args;
    int status;
    char cwd[PATH_MAX];

    do {
        getcwd(cwd, sizeof(cwd));
        printf("current directory: %s\n> ", cwd);
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
        printf("\n");
    } while (status);
}


int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    lsh_loop();
}