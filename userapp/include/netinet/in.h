#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <stdint.h>

static __inline uint16_t
htons(uint16_t x)
{
#ifdef __BIG_ENDIAN__
	return x;
#else
	uint8_t *s = (uint8_t *)&x;
	return (uint16_t)(s[0] << 8 | s[1]);
#endif
}

static __inline uint16_t
ntohs(uint16_t x)
{
#ifdef __BIG_ENDIAN__
	return x;
#else
	uint8_t *s = (uint8_t *)&x;
	return (uint16_t)(s[0] << 8 | s[1]);
#endif
}

static __inline uint32_t
htonl(uint32_t x)
{
#ifdef __BIG_ENDIAN__
	return x;
#else
	uint8_t *s = (uint8_t *)&x;
	return (uint32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#endif
}

static __inline uint32_t
ntohl(uint32_t x)
{
#ifdef __BIG_ENDIAN__
	return x;
#else
	uint8_t *s = (uint8_t *)&x;
	return (uint32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#endif
}

#endif /*_NETINET_IN_H*/
