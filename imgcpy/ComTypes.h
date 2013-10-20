#ifndef _COMTYPES_H_
#define _COMTYPES_H_

#include <stddef.h>

typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned int uint;
typedef unsigned short int ushort;
typedef unsigned long int ulong;

typedef signed char int8;
typedef unsigned char uint8;

typedef signed short int int16;
typedef unsigned short int uint16;

typedef signed long int int32;
typedef unsigned long int uint32;

#if defined(__DJGPP__) || defined(__GNUC__)
typedef signed long long int int64;
typedef unsigned long long int uint64;
#endif
#if defined(_MSC_VER) || defined(__WATCOMC__)
typedef __int64 int64;
typedef unsigned __int64 uint64;
#endif

typedef union tBigType {
  size_t  TypeSizeT;
  ulong   TypeULong;
  uint    TypeUInt;
  uchar   TypeUChar;
  wchar_t TypeWChar;
  void*   TypePtr;
  void(*  TypeFxnPtr)();
} tBigType;

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

#endif // _COMTYPES_H_
