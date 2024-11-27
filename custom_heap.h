#ifndef CUSTOM_HEAP_H
#define CUSTOM_HEAP_H

#include <stddef.h>

void initialize_memory_pool(size_t size);
void *allocate_block(size_t size);
void release_block(void *ptr);
void *resize_block(void *ptr, size_t size);

#endif // CUSTOM_HEAP_H