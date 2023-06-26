CFLAGS = -std=gnu11 -g -Wall -Wextra -pedantic
CC = gcc
CLIBS = -lraylib -lm

main: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)
