#ifndef _COMMON_DEFINES_H_
#define _COMMON_DEFINES_H_

#include <stddef.h>

// MIN() and MAX() macros:
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// ABS() and SGN() macros:
#define ABS(a) (((a)<0)?-(a):(a))
#define SGN(a) (((a)<0)?-1:(((a)>0)?1:0))

// This macro returns offset of a member of a structure
#define GetMemberOffset(StructType,MemberName)\
  ((size_t)&((StructType*)0)->MemberName)

// This macro returns number of elements contained in an array
#define GetArrayElementCount(Array)\
  (sizeof(Array)/sizeof(Array[0]))

// This macro is a compile-time "assert" that can be used to
// validate some things that can't be evaluated at preprocessing
// time like sizeof(type).
#define C_ASSERT(x) extern char __aSsErT__[1-2*!(x)];

#ifdef __TURBOC__
 #ifdef inline
  #undef inline
 #endif
 #define inline
#endif

#if defined (__GNUC__) || defined(__386__)
 #ifdef far
  #undef far
 #endif
 #define far
#endif

#endif // _COMMON_DEFINES_H_
