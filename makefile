CFLAGS = -std=c17 -g -Wall -Wextra -pedantic
CC = cc
CLIBS = -lraylib -lm

chip8: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)
