#ifndef _STDLIB_H
#define _STDLIB_H

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

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif /* _STDLIB_H */
