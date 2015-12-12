#include <string.h>

int
memcmp(const void *b1, const void *b2, size_t len)
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

void *memmove(void *dest, const void *src, size_t count)
{
	char *tmp;
	const char *s;

	if (dest <= src) {
		tmp = dest;
		s = src;
		while (count--)
			*tmp++ = *s++;
	} else {
		tmp = dest;
		tmp += count;
		s = src;
		s += count;
		while (count--)
			*--tmp = *--s;
	}
	return dest;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
		if ((unsigned char)c == *p++) {
			return (void *)(p - 1);
		}
	}
	return NULL;
}

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

int strncmp(const char *cs, const char *ct, size_t count)
{
	 unsigned char c1, c2;

	 while (count) {
		 c1 = *cs++;
		 c2 = *ct++;
		 if (c1 != c2)
			 return c1 < c2 ? -1 : 1;
		 if (!c1)
			 break;
		 count--;
	 }
	 return 0;
}

char *strchr(const char *s, int c)
{
   for (; *s != (char)c; ++s)
	if (*s == '\0')
		return NULL;
	return (char *)s;
}

char *strrchr(const char *s, int c)
{
	const char *last = NULL;
	do {
		if (*s == (char)c)
			last = s;
	} while (*s++);
	return (char *)last;
}

char *strstr(const char *s1, const char *s2)
{
	size_t l1, l2;

	l2 = strlen(s2);
	if (!l2)
			return (char *)s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}

char *
strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from) != 0; ++from, ++to);
	return(save);
}

size_t
strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s);
	return(s - str);
}

char *
strncpy(char *dest, const char *src, size_t n)
{
    unsigned i;

   for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';

   return dest;
}
