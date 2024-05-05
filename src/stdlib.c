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

void*
memmove(void *dst, const void *src, size_t n)
{
  const char *s;
  char *d;

  if(n == 0)
    return dst;
  
  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}