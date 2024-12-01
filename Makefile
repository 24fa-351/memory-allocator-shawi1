# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -DDEBUG

# Target for testing the custom heap manager implementation
test_custom_heap: test_custom_heap.c custom_heap.c custom_heap.h
	$(CC) $(CFLAGS) -DUSE_CUSTOM -o test_custom_heap test_custom_heap.c custom_heap.c -lpthread

# Target for testing the system's malloc/free/realloc
test_system_heap: test_custom_heap.c
	$(CC) $(CFLAGS) -o test_system_heap test_custom_heap.c -lpthread

# Clean up generated files
clean:
	rm -f test_custom_heap test_system_heap