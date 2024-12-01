#include <stdio.h>
#include <stdlib.h>
#include "custom_heap.h"

// Use custom memory functions if USE_CUSTOM is defined
#ifdef USE_CUSTOM
#define malloc(size) allocate_block(size)
#define free(ptr) release_block(ptr)
#define realloc(ptr, size) resize_block(ptr, size)
#endif

void test_malloc_free()
{
    printf("Running malloc/free test...\n");
    void *ptr1 = malloc(128);
    void *ptr2 = malloc(256);
    if (!ptr1 || !ptr2)
    {
        printf("malloc/free test FAILED (malloc returned NULL)\n");
        return;
    }
    free(ptr1);
    free(ptr2);
    printf("malloc/free test PASSED\n");
}

void test_realloc()
{
    printf("Running realloc test...\n");
    void *ptr = malloc(128);
    if (!ptr)
    {
        printf("realloc test FAILED (malloc returned NULL)\n");
        return;
    }
    ptr = realloc(ptr, 256);
    if (!ptr)
    {
        printf("realloc test FAILED (realloc returned NULL)\n");
        return;
    }
    free(ptr);
    printf("realloc test PASSED\n");
}

int main()
{
#ifdef USE_CUSTOM
    printf("Using custom heap manager.\n");
    initialize_memory_pool(2048); // Initialize the custom heap with 2048 bytes
#else
    printf("Using system malloc/free/realloc.\n");
#endif

    test_malloc_free();
    test_realloc();

    return 0;
}