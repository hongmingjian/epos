#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include "global.h"

/*
 * Utilities
 */
void *memcpy(void *, const void *, unsigned long);
void *memset(void *, int, unsigned long);
int   memcmp(const void *, const void *, unsigned long);
short cksum(const void *, unsigned long);
short htons(short);
short ntohs(short);
long  htonl(long);
long  ntohl(long);

char *strcat(char *s, const char *append);
int   strcmp(const char *s1, const char *s2);
char *strcpy(char *to, const char *from);
unsigned strlen(const char *str);
char *strncpy(char *dest, const char *src, unsigned n);
int   isdigit(char c);
int   isxdigit(char c);
int   islower(char c);
int   isupper(char c);
int   isalpha(char c);
int   isalnum(char c);
int   isspace(char c);
int   atoi(const char *p);
int   htoi(char *ptr);
long  htol(char *ptr);
char toupper(char c);
char tolower(char c);
size_t strnlen(const char *str, size_t count);
int strncmp(const char *s1, const char *s2, size_t n);

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

uint64_t __udivmoddi4(uint64_t num,
                      uint64_t den,
                      uint64_t *rem_p);
int64_t __divdi3(int64_t num, int64_t den);
int64_t __moddi3(int64_t num, int64_t den);
uint64_t __udivdi3(uint64_t num, uint64_t den);
uint64_t __umoddi3(uint64_t num, uint64_t den);

#endif /*_UTILS_H*/
