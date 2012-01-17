// UCLA CS 111 Lab 1 storage allocation

#include <stddef.h>

// Return a pointer to memory of size size_t
void *checked_malloc (size_t);

// Reallocate memory for the area pointed to by void* to size size_t
void *checked_realloc (void *, size_t);


void *checked_grow_alloc (void *, size_t *);
