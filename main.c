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
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    /*
    strtok returns a token. It remembers the position of the last token it returned, so that it can return the next token automatically.
    */
    token = strtok(line, LSH_TOK_DELIM); 
    
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

#define LSH_COM_BUFSIZE 32

/**
 * @brief lsh_pipe_split_line splits the given line including pipe operators into multiple commands.
 * 
 * @param line the given line
 * @param ncmds the number of commands. This function expects it is zero at first, and tells it the number of commands.
 * @return char*** An array of commands. Each command is comprised of arguments.
 */
char ***lsh_pipe_split_line(char *line, int *ncmds)
{
    *ncmds = 0;

    int command_bufsize = LSH_COM_BUFSIZE;
    char ***commands = malloc((size_t)command_bufsize * sizeof(char **));
    if (!commands) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    int command_position = 0;

    char *token = strtok(line, LSH_TOK_DELIM);

    while (token != NULL) {
        int token_bufsize = LSH_TOK_BUFSIZE;
        char **tokens = malloc((size_t)token_bufsize * sizeof(char *));
        int token_position = 0;
        if (!tokens) {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }

        while (token != NULL && strcmp(token, "|") != 0) {
            tokens[token_position++] = token;

            if (token_position >= token_bufsize) {
                token_bufsize += LSH_TOK_BUFSIZE;
                tokens = realloc(tokens, (size_t)token_bufsize * sizeof(char *));
                if (!tokens) {
                    fprintf(stderr, "lsh: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }

            token = strtok(NULL, LSH_TOK_DELIM);
        }

        tokens[token_position] = NULL;
        commands[command_position++] = tokens;
        (*ncmds)++;

        if (command_position >= command_bufsize) {
            command_bufsize += LSH_COM_BUFSIZE;
            char ***tmp =
                realloc(commands, (size_t)command_bufsize * sizeof(char **));
            if (!tmp) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
            commands = tmp;
        }

        if (token != NULL && strcmp(token, "|") == 0)
            token = strtok(NULL, LSH_TOK_DELIM);
    }

    return commands;
}


/**
 * @brief lsh_pipe_launch launches multiple commands tied with pipe operators.
 * 
 * @param commands the given commands
 * @param ncmds the number of commands
 * @return int status code
 */
int lsh_pipe_launch(char ***commands, int ncmds)
{
    if (ncmds <= 0 || commands == NULL)
        return 1;

    /* Single command — should not reach here normally; behave like fork/exec anyway. */
    if (ncmds == 1) {
        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(commands[0][0], commands[0]) == -1) {
                perror("lsh");
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_FAILURE);
        }
        if (pid < 0) {
            perror("lsh");
            return -1;
        }
        int status;
        waitpid(pid, &status, 0);
        (void)status;
        return 1;
    }

    pid_t *pids = malloc((size_t)ncmds * sizeof(pid_t));
    if (!pids) {
        fprintf(stderr, "lsh: allocation error\n");
        return -1;
    }

    int prev_read = -1;
    pid_t pid, wpid;
    int status;

    for (int i = 0; i < ncmds; i++) {
        int cur[2] = {-1, -1};
        if (i < ncmds - 1) {
            if (pipe(cur) < 0) {
                perror("lsh");
                free(pids);
                return -1;
            }
        }

        pid = fork();
        if (pid == 0) {
            if (prev_read != -1) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (cur[1] != -1) {
                dup2(cur[1], STDOUT_FILENO);
                close(cur[1]);
                close(cur[0]);
            }

            if (execvp(commands[i][0], commands[i]) == -1) {
                perror("lsh");
            }
            _exit(EXIT_FAILURE);
        }

        if (pid < 0) {
            perror("lsh");
            if (prev_read != -1)
                close(prev_read);
            if (cur[0] != -1) {
                close(cur[0]);
                close(cur[1]);
            }
            free(pids);
            return -1;
        }

        pids[i] = pid;
        if (prev_read != -1)
            close(prev_read);
        if (cur[1] != -1)
            close(cur[1]);
        prev_read = cur[0];
    }

    if (prev_read != -1)
        close(prev_read);

    for (int i = 0; i < ncmds; i++) {
        do {
            wpid = waitpid(pids[i], &status, WUNTRACED);
            if (wpid == -1) {
                perror("lsh");
                free(pids);
                return -1;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    free(pids);
    return 1;
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
        exit(EXIT_FAILURE); // This lin
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
            // This condition checks if the child process truly ends or not.
            // Since waitpid() returns a value when the child process just stops temporarily,
            // and we have to wait for that it resumes and truly ends.
            // If it does not end, then it goes to next iteration and call waitpid() again.
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
    // If the given function name is built-in, then just call it.
    for (int i = 0; i < lsh_num_builtins(); i++) {
            // comparing string
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i]) (args);
        } 
    }
    // If not built-in, then use fork() and execvp() to start new process.
    return lsh_launch(args);
}

/**
 * @brief lsh_is_builtin checks if the given function name is built-in or not.
 * 
 * @param name the given function name
 * @return int status code
 */
int lsh_is_builtin(const char *name)
{
    if (!name) return 0;
    for (int j = 0; j < lsh_num_builtins(); j++)
        if (strcmp(name, builtin_str[j]) == 0)
            return 1;
    return 0;
}

/**
 * @brief lsh_pipe_execute executes the given commands with pipe operators with making sure they do not include built-in functions.
 * 
 * @param commands the given commands
 * @param ncmds the number of commands
 * @return int status code
 */
int lsh_pipe_execute(char ***commands, int ncmds)
{
    for (int i = 0; i < ncmds; i++) {
        if (lsh_is_builtin(commands[i][0])) {
            fprintf(stderr, "lsh: built-ins cannot be used in pipelines\n");
            return -1;
        }
    }
    return lsh_pipe_launch(commands, ncmds);
}


/**
 * @brief lsh_pipe_exists judges if any pipe operator exists or not.
 * 
 * @param line the given line
 * @return int status code
 */
int lsh_pipe_exists(char *line)
{
    for (int i = 0; i < (int)strlen(line); i++) {
        if (line[i] == '|') {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief This function takes user's input and execute commands.
 * 
 */
void lsh_loop(void)
{
    char *line;
    char **args;
    char ***commands;
    int ncmds;
    int status = 1;
    char cwd[PATH_MAX];

    do {
        args = NULL;
        commands = NULL;
        getcwd(cwd, sizeof(cwd));
        printf("current directory: %s\n> ", cwd);

        line = lsh_read_line();

        if (lsh_pipe_exists(line)) {
            commands = lsh_pipe_split_line(line, &ncmds);
            status = lsh_pipe_execute(commands, ncmds);
            for (int i = 0; i < ncmds; i++)
                free(commands[i]);
            free(commands);
        } else {
            args = lsh_split_line(line);
            status = lsh_execute(args);
            free(args);
        }

        free(line);
        printf("\n");
    } while (status);
}


int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    lsh_loop();
}