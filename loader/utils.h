#ifndef UTILS_H
#define UTILS_H 1

#include <sys/types.h>
#include <stdarg.h>

#define	LSB(u)		((u) & 0xFF)
#define	MSB(u)		((u) >> 8)

#define	ENTRIES(a)	(sizeof(a)/sizeof(a[0]))

#ifndef __offsetof
#define __offsetof(type, field) ((size_t)(&((type *)0)->field))
#endif

#ifndef NULL
#define	NULL		((void *) 0)
#endif

#ifndef EOF
#define	EOF		(-1)
#endif


typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/*
 * Utilities
 */
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

int sprintf(char * buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

int puts(const char *s);
int printk(const char *fmt,...);

#endif /*UTILS_H*/
