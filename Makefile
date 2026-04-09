CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = myshell
SRC = $(wildcard src/*.c) $(wildcard src/custom/*.c)
SRC := $(filter-out src/custom/test_program.c,$(SRC))
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all run clean
