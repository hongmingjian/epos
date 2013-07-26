#include "utils.h"

void *
memcpy(void *dst, const void *src, unsigned long len)
{
	char *p1, *p2;
	p1 = (char *)dst;
	p2 = (char *)src;

	while(len--) {
		*p1++ = *p2++;
	}
	return dst;
}

void *
memset(void *b, int c, unsigned long len)
{
	char *p;
	p = (char *)b;

	while(len--) {
		*p++ = c;
	}
	return b;
}

int
memcmp(const void *b1, const void *b2, unsigned long len)
{
	char *p1, *p2;

	if(len == 0)
		return len;

	p1 = (char *)b1;
	p2 = (char *)b2;

	do {
		if(*p1++ != *p2++)
			break;
	} while(--len);

	return len;
}

short
cksum(const void *b, unsigned long len)
{
	unsigned long sum;
	unsigned short *p;

	sum = 0;
	p = (unsigned short *)b;

	while(len > 1) {
		sum += *p++;
		len -= 2;
	}

	if(len)
		sum += *(unsigned char *)p;

	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);

	return ~sum;
}

short
htons(short x)
{
#ifdef __BIG_ENDIAN__	
	return x;
#else
	unsigned char *s = (unsigned char *)&x;
	return (short)(s[0] << 8 | s[1]);
#endif
}

short
ntohs(short x)
{
#ifdef __BIG_ENDIAN__	
	return x;
#else
	unsigned char *s = (unsigned char *)&x;
	return (short)(s[0] << 8 | s[1]);
#endif
}

long
htonl(long x)
{
#ifdef __BIG_ENDIAN__	
	return x;
#else
	unsigned char *s = (unsigned char *)&x;
	return (long)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#endif
}

long
ntohl(long x)
{
#ifdef __BIG_ENDIAN__	
	return x;
#else
	unsigned char *s = (unsigned char *)&x;
	return (long)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#endif
}



/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

char *
strcat(char *s, const char *append)
{
	char *save = s;

	for(; *s; ++s);
	while((*s++ = *append++) != 0);
	return(save);
}

int
strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

char *
strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from) != 0; ++from, ++to);
	return(save);
}

unsigned
strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s);
	return(s - str);
}

unsigned long 
strlcpy(char *dest, const char *src, unsigned long len) 
{ 
	unsigned long src_len = strlen(src); 
	unsigned long new_len; 

	if (len == 0) {
		return (src_len);
	}

	if (src_len >= len) {
		new_len = len - 1;
	} else {
		new_len = src_len;
	}

	memcpy(dest, src, new_len); 
	dest[new_len] = '\0'; 
	return (src_len); 
}

#define	ISDIGIT(_c) \
	((_c) >= '0' && (_c) <= '9')

#define	ISXDIGIT(_c) \
	(ISDIGIT(_c) || \
	((_c) >= 'a' && (_c) <= 'f') || \
	((_c) >= 'A' && (_c) <= 'F'))

#define	ISLOWER(_c) \
	((_c) >= 'a' && (_c) <= 'z')

#define	ISUPPER(_c) \
	((_c) >= 'A' && (_c) <= 'Z')

#define	ISALPHA(_c) \
	(ISUPPER(_c) || \
	ISLOWER(_c))

#define	ISALNUM(_c) \
	(ISALPHA(_c) || \
	ISDIGIT(_c))

#define	ISSPACE(_c) \
	((_c) == ' ' || \
	(_c) == '\t' || \
	(_c) == '\r' || \
	(_c) == '\n')

inline int
isdigit(char c)
{
	return (ISDIGIT(c));
}


inline int
isxdigit(char c)
{
	return (ISXDIGIT(c));
}


inline int
islower(char c)
{
	return (ISLOWER(c));
}


inline int
isupper(char c)
{
	return (ISUPPER(c));
}


inline int
isalpha(char c)
{
	return (ISALPHA(c));
}


inline int
isalnum(char c)
{
	return (ISALNUM(c));
}


inline int
isspace(char c)
{
	return (ISSPACE(c));
}

int
atoi(const char *p)
{
	int n;
	int c, neg = 0;
	unsigned char	*up = (unsigned char *)p;

	if (!isdigit(c = *up)) {
		while (isspace(c))
			c = *++up;
		switch (c) {
		case '-':
			neg++;
			/* FALLTHROUGH */
		case '+':
			c = *++up;
		}
		if (!isdigit(c))
			return (0);
	}
	for (n = '0' - c; isdigit(c = *++up); ) {
		n *= 10; /* two steps to avoid unnecessary overflow */
		n += '0' - c; /* accum neg to avoid surprises at MAX */
	}
	return (neg ? n : -n);
}

static int 
hex(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

int
htoi(char *ptr)
{
	int hv; int rv = 0;

	while (*ptr) {
		hv = hex(*ptr);
		if (hv < 0)
			break;
		rv = (rv << 4) | hv;
		
		ptr++;
	}
	return rv;
}

long
htol(char *ptr)
{
	int hv; long rv = 0;

	while (*ptr) {
		hv = hex(*ptr);
		if (hv < 0)
			break;
		rv = (rv << 4) | hv;
		
		ptr++;
	}
	return rv;
}

char
toupper(char c)
{
	return ((c >= 'a' && c <= 'z') ? c - 32: c);
}

char
tolower(char c)
{
	return ((c >= 'A' && c <= 'Z') ? c + 32: c);
}

size_t
strnlen(const char *str, size_t count)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s && count; ++s, count--);
  return s-str;
}

int
strncmp(const char *s1, const char *s2, size_t n)
{
    for ( ; n > 0; s1++, s2++, --n)
	if (*s1 != *s2)
	    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
	else if (*s1 == '\0')
	    return 0;
    return 0;
}

