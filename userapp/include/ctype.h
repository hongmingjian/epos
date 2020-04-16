#ifndef _CTYPE_H
#define _CTYPE_H

static __inline int islower (int c) { return c >= 'a' && c <= 'z'; }
static __inline int isupper (int c) { return c >= 'A' && c <= 'Z'; }
static __inline int isalpha (int c) { return islower (c) || isupper (c); }
static __inline int isdigit (int c) { return c >= '0' && c <= '9'; }
static __inline int isalnum (int c) { return isalpha (c) || isdigit (c); }
static __inline int isxdigit (int c) {
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static __inline int isspace (int c) {
  return (c == ' ' || c == '\f' || c == '\n'
          || c == '\r' || c == '\t' || c == '\v');
}
static __inline int isblank (int c) { return c == ' ' || c == '\t'; }
static __inline int isgraph (int c) { return c > 32 && c < 127; }
static __inline int isprint (int c) { return c >= 32 && c < 127; }
static __inline int iscntrl (int c) { return (c >= 0 && c < 32) || c == 127; }
static __inline int isascii (int c) { return c >= 0 && c < 128; }
static __inline int ispunct (int c) {
  return isprint (c) && !isalnum (c) && !isspace (c);
}

static __inline int tolower (int c) { return isupper (c) ? c - 'A' + 'a' : c; }
static __inline int toupper (int c) { return islower (c) ? c - 'a' + 'A' : c; }

#endif /* _CTYPE_H */
