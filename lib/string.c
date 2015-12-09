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
