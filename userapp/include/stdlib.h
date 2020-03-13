#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/types.h>

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef struct _div_t {
  int quot;
  int rem;
} div_t;

typedef struct _ldiv_t {
  long quot;
  long rem;
} ldiv_t;

div_t div (int numer, int denom);
ldiv_t ldiv (long int numer, long int denom);

#define	RAND_MAX	0x7ffffffd
void srand(unsigned int seed);
int  rand();
int  rand_r(unsigned int *seedp);

void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *oldptr, size_t size);
void  free(void *ptr);
#ifdef  __GNUC__
# define alloca(size) __builtin_alloca (size)
#endif /*__GNUC__*/

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0
void exit(int);

long strtol(const char *nptr, char **endptr, register int base);
unsigned long strtoul(const char *nptr, char **endptr, register int base);
long atol(const char *str);
char *strdup (const char *s);

void qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *));

#endif /* _STDLIB_H */
