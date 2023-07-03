CFLAGS = -std=c17 -g -Wall -Wextra -pedantic
CC = cc
CLIBS = -lraylib -lm

main: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)
