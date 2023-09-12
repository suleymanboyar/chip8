CFLAGS = -std=c17 -g -Wall -Wextra -pedantic
CC = cc
CLIBS = -lm
CLIBS += `pkg-config --libs raylib`

chip8: main.c
	$(CC) $(CFLAGS) $(CLIBS) $^ -o $@
