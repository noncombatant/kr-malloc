#include <stddef.h>

// Returns a pointer to a memory region containing at least `count * size`
// bytes. Checks the multiplication for overflow.
//
// Returns `NULL` and sets `errno` if there was an error.
void* kr_malloc(size_t count, size_t size);

// Puts the memory region that `p` points to back onto the free list.
void kr_free(void* p);
