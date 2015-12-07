#ifndef _STRING_H
#define _STRING_H

void *memcpy(void *dst, const void *src, unsigned long len);
void *memset(void *b, int c, unsigned len);
int memcmp(const void *b1, const void *b2, unsigned long len);

char *strcat(char *s, const char *append);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *to, const char *from);
unsigned strlen(const char *str);
char *strncpy(char *dest, const char *src, unsigned n);

#endif /* _STRING_H */
