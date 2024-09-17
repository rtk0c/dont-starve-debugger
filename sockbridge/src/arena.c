#include "arena.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct Arena {
    size_t capacity;
    size_t fill;
    char buffer[4096*32767 - sizeof(size_t)*2];
};

Arena* make_arena() {
    // Use Linux's overcommit
    Arena* a = malloc(sizeof(Arena));
    a->capacity = 0;
    a->fill = 0;    
    return a;
}

void destroy_arena(Arena* a) {
    free(a);
}

char* arena_push(Arena* a, size_t size, size_t align) {
    uintptr_t align_mask = align - 1;
    // Pointer to first byte of unused memory
    uintptr_t initial = (uintptr_t)a->buffer + a->fill;
    // Round up by alignment
    uintptr_t rounded = (initial + align_mask) & ~align_mask;
    a->fill += rounded - initial + size;
    return (char*)rounded;
}

char* arena_push_zero(Arena* a, size_t size, size_t align) {
    char* res = arena_push(a, size, align);
    memset(res, 0, size);
    return res;
}

void arena_pop(Arena* a, size_t size) {
    // FIXME if something gets rounded by up alignment, we have no way to know
    a->fill -= size;
}
