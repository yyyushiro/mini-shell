# English Ver.
## Introduction
I made a mini-shell for deepening the understanding of the shell.

For the implementation of main functionalities, I followed [Tutorial-Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/).

This tutorial offers how to get arguments and call built-in functions or external functions.

Then, I implemented the following functionalities by myself;

- Displaying the current directory all the time.
- Backslash(\\) escaping. 
- pipe operators (only for non-built-in functions).

## How Shell works(basic)
Shell is just a single process that takes the user's input and executes commands.

There are two types of commands; external command and builtin command.

The way of executing them depends on whether they are external or builtin.

### External
External commands are binary files. Therefore, each external command is a single process and has to work separete from the original shell process.

To run external commands, we use `fork()` and `execvp()` functions.

`fork()` creates a copy of the current process (in this case, the shell process) including memory allocation. To be exact, Copy on Write is implemented. 

`execvp()` replaces the copied shell process with the given process.

For example, if you want to execute `ls`, the current shell process first duplicate itself, and its child process becomes `ls` and `ls` outputs its result.

### Builtin
Since this is literally builtin, the shell just runs it(e.g. `cd`, `echo`, `pwd`).
These commands must be builtin. The reason for this is that they modify the state of the shell process.

For example, If `cd` were to be an external command, then `cd` just changes the current directory of the copied shell process, not the original one.

### Summary
What Shell does is faily simple. It just follows user's input and executes the two types of commands.

## Points of Implementation
### waitpid()
There is a type of processes called zombie process.

They are born when their parent processes do not check if they are finished or not while they actually finished.

While the resources like file descriptor or memory are released, OS still manages the process id for these zombie processes, leading to PID exhausion.

To settle this issue, we have to use `waitpid()`. If `waitpid()` is used in a parent process, it waits its child process ends. Since it checks its child process ends, its zombie process never be born.

### Pipe Operator
Pipe Operator('|') is necessary for better use of Shell.

It connects the standard input/output of each command and makes the more flexible usage of commands possible.

For example, you can search a target file by `ls | grep "foo"`.

To implement the pipe operator, we use a unidirectional channel.

In UNIX, A unidirectional channel is represented as a pair of file descriptors.

One of them is for reading, and the other is for writing.

Along with this, an external command reads standard input, and write to standard output.

In addition, since standard input/output are also represented as file descriptors, we can tie our own file descriptor with the starndard input/output.

Therefore, if we tie each standard input/output with the input/output of each external command, The mutiple external commands can get from or send to another external command information.

For example, if you run `ls | grep "foo"`, `ls` sends its output to the input of `grep "foo"`. This is because the standard output of `ls` is tied to a write file descriptor, and the standard input of `grep "foo"` is tied to a read file descriptor. Note that the write file descriptor and the read file descriptor are a pair and form a unidirectional channel.

## Japanese Ver.







