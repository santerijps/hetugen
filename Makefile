CC 				:= gcc
CFLAGS			:= -Wall -Wextra -Wpedantic

DEBUG_FLAGS		:= -Og -g
RELEASE_FLAGS 	:= -Ofast -march=native -DNODEBUG -s

BIN_DIR			:= bin

.PHONY: debug release debug-win release-win

debug: bin
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) hetugen.c -o $(BIN_DIR)/hetugen.debug.exe

release: bin
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) hetugen.c -o $(BIN_DIR)/hetugen.exe

debug-win: bin
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) hetugen.win.c -o $(BIN_DIR)/hetugen.win.debug.exe

release-win: bin
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) hetugen.win.c -o $(BIN_DIR)/hetugen.win.exe

bin:
	mkdir bin
