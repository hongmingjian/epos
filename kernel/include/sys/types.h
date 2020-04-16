#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

typedef unsigned int size_t;
typedef int ssize_t;
typedef long time_t; /* time in seconds */
typedef long clock_t;/* time in ticks */

typedef long long off64_t;
#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS==64)
typedef off64_t off_t;
#else
typedef long off_t;
#endif

typedef int ptrdiff_t;

#endif /*_SYS_TYPES_H*/
