CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O0

all:
	$(MAKE) lsh

lsh: builtins.c builtins.h main.c
	$(CC) $(CFLAGS) main.c builtins.c -o lsh

clean:
	rm -f ./lsh