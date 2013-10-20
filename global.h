/**
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifndef __ASSEMBLY__
#define REGL(base, offset) ( *(volatile unsigned int *)(base + offset))
#define REGW(base, offset) ( *(volatile unsigned short *)(base + offset))
#define REGB(base, offset) ( *(volatile unsigned char *)(base + offset))
#else
#define REGL(base, offset) (base + offset)
#define REGW(base, offset) (base + offset)
#define REGB(base, offset) (base + offset)
#endif /* __ASSEMBLY__ */

#define __roundoff(x)  ( (int)(x + 0.5) )
#define __countof(x)  (sizeof(x) / sizeof( (x)[0] ) )

#define __CONCAT1(x,y)  x ## y
#define __CONCAT(x,y)   __CONCAT1(x,y)
#define __STRING(x)     #x              /* stringify without expanding x */
#define __XSTRING(x)    __STRING(x)     /* expand x, then stringify */

#ifndef __offsetof
#define __offsetof(type, field) ( (size_t)( &( (type *) 0) -> field) )
#endif

#define min(x,y)  ( ((x) > (y) ) ? (y) : (x) )
#define max(x,y)  ( ((x) < (y) ) ? (y) : (x) )

typedef unsigned char  uint8_t;
typedef          char   int8_t;
typedef unsigned short uint16_t;
typedef          short  int16_t;
typedef unsigned int   uint32_t;
typedef          int    int32_t;
typedef          long long  int64_t;
typedef unsigned long long uint64_t;

typedef unsigned int    size_t;
typedef          int    ssize_t;

#define NULL            ((void *)0)

#endif /* _GLOBAL_H */
