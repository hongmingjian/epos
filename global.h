#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifndef __ASSEMBLY__
#define	REGL(base, offset)	( *(volatile unsigned int *)(base + offset))
#define	REGW(base, offset)	( *(volatile unsigned short *)(base + offset))
#define	REGB(base, offset)	( *(volatile unsigned char *)(base + offset))
#else
#define	REGL(base, offset)	(base + offset)
#define	REGW(base, offset)	(base + offset)
#define	REGB(base, offset)	(base + offset)
#endif /* __ASSEMBLY__ */

#define __roundoff(x)		( (int)(x + 0.5) )
#define __countof(x)		(sizeof(x) / sizeof( (x)[0] ) )

#define __CONCAT1(x,y)		      x ## y
#define __CONCAT(x,y)		        __CONCAT1(x,y)
#define __STRING(x)             #x              /* stringify without expanding x */
#define __XSTRING(x)            __STRING(x)     /* expand x, then stringify */
#define __offsetof(type, field) ( (size_t)( &( (type *) 0) -> field) )

#define min(x,y)			  ( ((x) > (y) ) ? (y) : (x) )
#define max(x,y)			  ( ((x) < (y) ) ? (y) : (x) )

typedef unsigned char  uint8_t;
typedef          char   int8_t;
typedef unsigned short uint16_t;
typedef          short  int16_t;
typedef unsigned int   uint32_t;
typedef          int    int32_t;

typedef unsigned int    size_t;
typedef          int    ssize_t;

#define NULL            ((void *)0)

#endif /* _GLOBAL_H */
