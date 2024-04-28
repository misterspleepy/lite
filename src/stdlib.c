#include "type.h"
char *strcpy(char *dest, const char *src)
{
    char* cur = dest;
    while (*src) {
        *cur++ = *src++;
    }
    return dest;
}

void *memset(void *str, int c, size_t n)
{
    for (int i = 0; i < n; i++) {
        *((char*)str + i)= c;
    }
    return str;
}