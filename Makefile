CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src/main.c src/shell_loop.c src/parser.c src/executor.c
TARGET = myshell

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
