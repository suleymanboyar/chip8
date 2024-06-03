CFLAGS = -std=c17 -g -Wall -Wextra -pedantic
CC = cc
CLIBS += -I./lib/raylib/include
CLIBS += -L./lib/raylib/lib
CLIBS += -l:libraylib.a -lm -ldl -lpthread

chip8: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)
