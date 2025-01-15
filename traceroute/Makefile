# Final output file name
TARGET = traceroute

# Source and header files
SRC = traceroute.c
HDR = traceroute.h

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Default rule
all: $(TARGET)

# Rule to compile the program
$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Rule to clean up generated files
clean:
	rm -f $(TARGET)

# Rule to run the program
run: $(TARGET)
	./$(TARGET)

# PHONY rule to prevent conflicts with files of the same name
.PHONY: all clean run
