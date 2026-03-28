all:
	make lsh

lsh: builtins.c builtins.h main.c
	cc -std=c11 -Wall -Wextra -Werror main.c builtins.c -o lsh

clean:
	rm -f ./lsh