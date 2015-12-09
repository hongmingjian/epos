#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *b, int c, size_t len);
int memcmp(const void *b1, const void *b2, size_t len);

char *strcat(char *s, const char *append);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *to, const char *from);
size_t strlen(const char *str);
char *strncpy(char *dest, const char *src, size_t n);

#endif /* _STRING_H */
