# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Clean-up command
REMOVE = rm -f


# Program executable
OUTPUT = ping

# Source file
SRC = ping.c

# Default IP for testing
PING_TARGET = 8.8.8.8

# Special targets that are not files
.PHONY: build all run clean trace

# Default build target
build: $(OUTPUT)

# Alias for build target
all: build

# Build the executable in one step (no intermediate .o file)
$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Run the program as sudo
run: $(OUTPUT)
	sudo ./$< $(PING_TARGET)

# Run the program with system trace
trace: $(OUTPUT)
	sudo strace ./$< $(PING_TARGET)

# Clean all generated files
clean:
	$(REMOVE) $(OUTPUT)
