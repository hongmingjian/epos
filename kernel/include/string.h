#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <sys/types.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *b, int c, size_t len);
int memcmp(const void *b1, const void *b2, size_t len);
void *memmove(void *dest, const void *src, size_t count);
void *memchr(const void *s, int c, size_t n);

char *strcat(char *s, const char *append);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *to, const char *from);
size_t strlen(const char *str);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *cs, const char *ct, size_t count);
char *strchr(const char *s, int c);
char *strstr(const char *s1, const char *s2);
char *strrchr(const char *s, int c);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t length);

#endif /* _STRING_H */
