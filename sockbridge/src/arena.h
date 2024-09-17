#pragma once

#include "str.h"

typedef struct Arena Arena;

Arena* make_arena();
void destroy_arena(Arena*);

char* arena_push(Arena*, size_t size, size_t align);
char* arena_push_zero(Arena*, size_t size, size_t align);
void arena_pop(Arena*, size_t size);

static inline
char* arena_push_zstring(Arena* a, String str) {
    char* res = arena_push(a, str.len + 1, 1);
    memcpy(res, str.data, str.len);
    res[str.len] = '\0';
    return res;
}

#define ARENA_DO_STRING_CONCAT(a, str) memcpy(arena_push((a), (str).len, 1), (str).data, (str).len)
#define ARENA_DO_LITERAL_CONCAT(a, literal) memcpy(arena_push((a), strlen(literal), 1), literal, strlen(literal))
#define ARENA_DO_NULL_TERMINATE(a) *arena_push((a), 1, 1) = '\0'