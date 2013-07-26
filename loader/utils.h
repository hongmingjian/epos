#ifndef _UTILS_H
#define _UTILS_H 1

#include "loader.h"
#include <stdarg.h>

void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int   memcmp(const void *, const void *, size_t);
short cksum(const void *, size_t);
short htons(short);
short ntohs(short);
long  htonl(long);
long  ntohl(long);

char *strcat(char *s, const char *append);
int   strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char *strcpy(char *to, const char *from);
size_t strlen(const char *str);
size_t strlcpy(char *dest, const char *src, size_t len); 
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

int puts(const char *s);
int printk(const char *fmt,...);

/* ------------------------------------------------------------ */
/* Support for functions implemented in vsprintf.c              */
/* ------------------------------------------------------------ */
int sprintf(char * buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

#endif /*UTILS_H*/
