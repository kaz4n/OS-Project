CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

PHASE1_TARGET = myshell
SERVER_TARGET = server
CLIENT_TARGET = client

PHASE1_SRC = $(wildcard src/*.c) $(wildcard src/custom/*.c)
PHASE1_SRC := $(filter-out src/server.c src/client.c src/custom/test_program.c,$(PHASE1_SRC))

SERVER_SRC = src/server.c src/parser.c src/executor.c src/shell_loop.c $(wildcard src/custom/*.c)
SERVER_SRC := $(filter-out src/custom/test_program.c,$(SERVER_SRC))

CLIENT_SRC = src/client.c

all: $(PHASE1_TARGET) $(SERVER_TARGET) $(CLIENT_TARGET)

$(PHASE1_TARGET): $(PHASE1_SRC)
	$(CC) $(CFLAGS) $(PHASE1_SRC) -o $(PHASE1_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_TARGET)

run-phase1: $(PHASE1_TARGET)
	./$(PHASE1_TARGET)

run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

clean:
	rm -f $(PHASE1_TARGET) $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all run-phase1 run-server run-client clean