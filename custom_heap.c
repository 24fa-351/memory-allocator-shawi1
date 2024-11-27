#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "custom_heap.h"

#ifdef DEBUG
#define LOG_INFO(msg, ...) fprintf(stderr, "[LOG] " msg "\n", ##__VA_ARGS__)
#else
#define LOG_INFO(msg, ...)
#endif

#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)

typedef struct memory_header
{
    size_t size;
    int is_free;
    struct memory_header *next;
} memory_header_t;

#define ALIGN4(size) (((size) + 3) & ~3)
#define HEADER_SIZE (sizeof(memory_header_t))

#define HEAP_MAX 512
typedef struct
{
    memory_header_t *blocks[HEAP_MAX];
    size_t count;
} min_heap_t;

// Global variables
static void *memory_pool = NULL;
static size_t memory_pool_size = 0;
static min_heap_t free_heap;
static pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

static void heap_initialize(min_heap_t *heap)
{
    heap->count = 0;
}

static void heap_push(min_heap_t *heap, memory_header_t *block)
{
    if (heap->count >= HEAP_MAX)
    {
        LOG_ERROR("Heap overflow!");
        return;
    }
    size_t idx = heap->count++;
    while (idx > 0)
    {
        size_t parent_idx = (idx - 1) / 2;
        if (heap->blocks[parent_idx]->size <= block->size)
            break;
        heap->blocks[idx] = heap->blocks[parent_idx];
        idx = parent_idx;
    }
    heap->blocks[idx] = block;
}

static memory_header_t *heap_pop(min_heap_t *heap)
{
    if (heap->count == 0)
        return NULL;
    memory_header_t *min_block = heap->blocks[0];
    memory_header_t *last_block = heap->blocks[--heap->count];
    size_t idx = 0;
    while (2 * idx + 1 < heap->count)
    {
        size_t left = 2 * idx + 1;
        size_t right = left + 1;
        size_t smallest = (right < heap->count && heap->blocks[right]->size < heap->blocks[left]->size) ? right : left;
        if (last_block->size <= heap->blocks[smallest]->size)
            break;
        heap->blocks[idx] = heap->blocks[smallest];
        idx = smallest;
    }
    heap->blocks[idx] = last_block;
    return min_block;
}

void initialize_memory_pool(size_t size)
{
    pthread_mutex_lock(&heap_mutex);
    memory_pool = malloc(size);
    if (!memory_pool)
    {
        LOG_ERROR("Failed to initialize memory pool.");
        pthread_mutex_unlock(&heap_mutex);
        exit(EXIT_FAILURE);
    }
    memory_pool_size = size;
    memory_header_t *initial_block = (memory_header_t *)memory_pool;
    initial_block->size = size - HEADER_SIZE;
    initial_block->is_free = 1;
    initial_block->next = NULL;
    heap_initialize(&free_heap);
    heap_push(&free_heap, initial_block);
    LOG_INFO("Memory pool initialized with %zu bytes.", size);
    pthread_mutex_unlock(&heap_mutex);
}

void *allocate_block(size_t size)
{
    pthread_mutex_lock(&heap_mutex);
    size = ALIGN4(size);
    memory_header_t *block = heap_pop(&free_heap);
    if (!block || block->size < size)
    {
        LOG_ERROR("No block available for size %zu.", size);
        pthread_mutex_unlock(&heap_mutex);
        return NULL;
    }
    if (block->size > size + HEADER_SIZE)
    {
        memory_header_t *new_block = (memory_header_t *)((char *)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->is_free = 1;
        new_block->next = NULL;
        heap_push(&free_heap, new_block);
        block->size = size;
    }
    block->is_free = 0;
    void *memory = (char *)block + HEADER_SIZE;
    memset(memory, 0, size);
    LOG_INFO("Allocated %zu bytes at %p.", size, memory);
    pthread_mutex_unlock(&heap_mutex);
    return memory;
}

void release_block(void *ptr)
{
    if (!ptr)
        return;
    pthread_mutex_lock(&heap_mutex);
    memory_header_t *block = (memory_header_t *)((char *)ptr - HEADER_SIZE);
    block->is_free = 1;
    heap_push(&free_heap, block);
    LOG_INFO("Freed block at %p.", ptr);
    pthread_mutex_unlock(&heap_mutex);
}

void *resize_block(void *ptr, size_t size)
{
    if (!ptr)
        return allocate_block(size);
    if (size == 0)
    {
        release_block(ptr);
        return NULL;
    }
    memory_header_t *block = (memory_header_t *)((char *)ptr - HEADER_SIZE);
    if (block->size >= size)
        return ptr;
    void *new_ptr = allocate_block(size);
    if (new_ptr)
    {
        memcpy(new_ptr, ptr, block->size);
        release_block(ptr);
    }
    LOG_INFO("Reallocated block from %p to %p with %zu bytes.", ptr, new_ptr, size);
    return new_ptr;
}