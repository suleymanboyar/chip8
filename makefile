CFLAGS = -std=gnu11 -g -Wall -Wextra -pedantic -Wl,-rpath=/usr/lib64
CC = gcc
CLIBS = -lraylib -lm


main: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)
