#pragma once

#include <string.h>

typedef struct String {
    char* data;
    size_t len;
} String;

static inline String str_from_cstring(char* in) {
    return (String){ in, strlen(in) };
}
