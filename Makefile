# Compiler to use
CC=gcc
# Compiler flags
CFLAGS=-Wall -Wextra -g

# Source files
SOURCES=main.c network_scan.c network_analysis.c utils.c
# Object files
OBJECTS=$(SOURCES:.c=.o)
# Executable name
EXECUTABLE=wifi_attack

all: $(EXECUTABLE)
	rm -f $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: clean
