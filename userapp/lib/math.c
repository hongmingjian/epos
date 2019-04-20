/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stdint.h>
#include <math.h>

double fabs(double x)
{
    if (x < 0)
        x = -x;
    return(x);
}

double floor(double x)
{
    return x < 0.f ? ((int)x - 1) : (int)x;
}

double ceil(double x)
{
    int ix = (int)x;
    if((double)ix == x)
        return ix;
    if(x < 0.f)
        return ix;
    else
        return ix+1;
}

double sin(double x)
{
    __asm__ ("fsin\n\t"
            : "+t"(x));
    return x;
}
double cos(double x)
{
    __asm__ ("fcos\n\t"
            : "+t"(x));
    return x;
}
double sqrt(double x)
{
    __asm__ ("fsqrt\n\t"
            : "+t"(x));
    return x;
}
double log2(double x, double y)
{
    __asm__ ("fyl2x\n\t"
            : "=t" (x)
            : "0" (x), "u" (y) : "st(1)");
    return x;
}
double atan2(double y,double x)
{
    __asm__ ("fpatan\n\t"
            : "=t" (x)
            : "0" (x), "u" (y) : "st(1)");
    return x;
}
double tan(double x)
{
    __asm__ ("fptan\n\t"
            "fxch\n\t"
            : "+t" (x));
    return x;
}
double cot(double x)
{
    __asm__ ("fptan\n\t"
            "fxch\n\t"
            "fdivrp\n\t"
            : "+t" (x));
    return x;
}

#if 1
/*https://github.com/emileb/OpenGames/blob/master/opengames/src/main/jni/Doom/gzdoom-g1.8.6/src/timidity/timidity.h*/
double pow(double x,double y)
{
	double result;

	if (y == 0) {
		return 1;
	}
	if (x == 0) {
		if (y > 0) {
			return 0;
		} else {
			union { double fp; long long ip; } infinity;
			infinity.ip = 0x7FF0000000000000ll;
			return infinity.fp;
		}
	}
	__asm__ (
		"fyl2x\n\t"
		"fld %%st(0)\n\t"
		"frndint\n\t"
		"fxch\n\t"
		"fsub %%st(1),%%st(0)\n\t"
		"f2xm1\n\t"
		"fld1\n\t"
		"faddp\n\t"
		"fxch\n\t"
		"fld1\n\t"
		"fscale\n\t"
		"fstp %%st(1)\n\t"
		"fmulp\n\t"
		: "=t" (result)
		: "0" (x), "u" (y)
		: "st(1)", "st(7)" );
	return result;
}
#else
/*http://www.willus.com/mingw/x87inline.h*/
double pow(double x,double y)
{
    double result;
    short t1,t2;
    __asm__ (
            "fxch\n\t"
            "ftst\n\t"
            "fstsw\n\t"
            "and $0x40,%%ah\n\t"
            "jz 1f\n\t"
            "fstp %%st(0)\n\t"
            "ftst\n\t"
            "fstsw\n\t"
            "fstp %%st(0)\n\t"
            "and $0x40,%%ah\n\t"
            "jnz 0f\n\t"
            "fldz\n\t"
            "jmp 2f\n\t"
            "0:\n\t"
            "fld1\n\t"
            "jmp 2f\n\t"
            "1:\n\t"
            "fstcw %3\n\t"
            "fstcw %4\n\t"
            "orw $0xC00,%4\n\t"
            "fldcw %4\n\t"
            "fld1\n\t"
            "fxch\n\t"
            "fyl2x\n\t"
            "fmulp\n\t"
            "fld %%st(0)\n\t"
            "frndint\n\t"
            "fxch\n\t"
            "fsub %%st(1),%%st(0)\n\t"
            "f2xm1\n\t"
            "fld1\n\t"
            "faddp\n\t"
            "fxch\n\t"
            "fld1\n\t"
            "fscale\n\t"
            "fstp %%st(1)\n\t"
            "fmulp\n\t"
            "fldcw %3\n\t"
            "2:"
            : "=t" (result)
            : "0" (y), "u" (x), "m" (t1), "m" (t2)
               : "st(1)", "st(7)", "%3", "%4", "ax" );
    return(result);
}
#endif

double exp(double x)
{
    double result;
    short t1,t2;
    __asm__ (
            "fstcw %2\n\t"
            "fstcw %3\n\t"
            "orw $0xC00,%3\n\t"
            "fldcw %3\n\t"
            "fldl2e\n\t"
            "fmulp\n\t"
            "fld %%st(0)\n\t"
            "frndint\n\t"
            "fxch\n\t"
            "fsub %%st(1),%%st(0)\n\t"
            "f2xm1\n\t"
            "fld1\n\t"
            "faddp\n\t"
            "fxch\n\t"
            "fld1\n\t"
            "fscale\n\t"
            "fstp %%st(1)\n\t"
            "fmulp\n\t"
            "fldcw %2"
            : "=t" (result)
            : "0" (x), "m" (t1), "m" (t2)
               : "st(6)", "st(7)", "%2", "%3");
    return(result);
}

double log(double x)
{
    return log2(x, 1.0)/1.442695040888963;
}

/* @(#)s_atan.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */
typedef union
{
  double value;
  struct
  {
    uint32_t lsw;
    uint32_t msw;
  } parts;
} ieee_double_shape_type;

#define GET_HIGH_WORD(i,d)			\
do {								\
  ieee_double_shape_type gh_u;		\
  gh_u.value = (d);					\
  (i) = gh_u.parts.msw;				\
} while (0)

#define GET_LOW_WORD(i,d)			\
do {								\
  ieee_double_shape_type gl_u;		\
  gl_u.value = (d);					\
  (i) = gl_u.parts.lsw;				\
} while (0)

/* atan(x)
 * Method
 *   1. Reduce x to positive by atan(x) = -atan(-x).
 *   2. According to the integer k=4t+0.25 chopped, t=x, the argument
 *      is further reduced to one of the following intervals and the
 *      arctangent of t is evaluated by the corresponding formula:
 *
 *      [0,7/16]      atan(x) = t-t^3*(a1+t^2*(a2+...(a10+t^2*a11)...)
 *      [7/16,11/16]  atan(x) = atan(1/2) + atan( (t-0.5)/(1+t/2) )
 *      [11/16.19/16] atan(x) = atan( 1 ) + atan( (t-1)/(1+t) )
 *      [19/16,39/16] atan(x) = atan(3/2) + atan( (t-1.5)/(1+1.5t) )
 *      [39/16,INF]   atan(x) = atan(INF) + atan( -1/t )
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */
static const double atanhi[] = {
  4.63647609000806093515e-01, /* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
  7.85398163397448278999e-01, /* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
  9.82793723247329054082e-01, /* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
  1.57079632679489655800e+00, /* atan(inf)hi 0x3FF921FB, 0x54442D18 */
};
static const double atanlo[] = {
  2.26987774529616870924e-17, /* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
  3.06161699786838301793e-17, /* atan(1.0)lo 0x3C81A626, 0x33145C07 */
  1.39033110312309984516e-17, /* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
  6.12323399573676603587e-17, /* atan(inf)lo 0x3C91A626, 0x33145C07 */
};
static const double aT[] = {
  3.33333333333329318027e-01, /* 0x3FD55555, 0x5555550D */
 -1.99999999998764832476e-01, /* 0xBFC99999, 0x9998EBC4 */
  1.42857142725034663711e-01, /* 0x3FC24924, 0x920083FF */
 -1.11111104054623557880e-01, /* 0xBFBC71C6, 0xFE231671 */
  9.09088713343650656196e-02, /* 0x3FB745CD, 0xC54C206E */
 -7.69187620504482999495e-02, /* 0xBFB3B0F2, 0xAF749A6D */
  6.66107313738753120669e-02, /* 0x3FB10D66, 0xA0D03D51 */
 -5.83357013379057348645e-02, /* 0xBFADDE2D, 0x52DEFD9A */
  4.97687799461593236017e-02, /* 0x3FA97B4B, 0x24760DEB */
 -3.65315727442169155270e-02, /* 0xBFA2B444, 0x2C6A6C2F */
  1.62858201153657823623e-02, /* 0x3F90AD3A, 0xE322DA11 */
};

static const double
one   = 1.0,
huge   = 1.0e300;

double atan(double x)
{
	double w,s1,s2,z;
	int32_t ix,hx,id;
	GET_HIGH_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x44100000) {	/* if |x| >= 2^66 */
	    uint32_t low;
	    GET_LOW_WORD(low,x);
	    if(ix>0x7ff00000||
		(ix==0x7ff00000&&(low!=0)))
		return x+x;		/* NaN */
	    if(hx>0) return  atanhi[3]+atanlo[3];
	    else     return -atanhi[3]-atanlo[3];
	} if (ix < 0x3fdc0000) {	/* |x| < 0.4375 */
	    if (ix < 0x3e200000) {	/* |x| < 2^-29 */
		if(huge+x>one) return x;	/* raise inexact */
	    }
	    id = -1;
	} else {
	x = fabs(x);
	if (ix < 0x3ff30000) {		/* |x| < 1.1875 */
	    if (ix < 0x3fe60000) {	/* 7/16 <=|x|<11/16 */
		id = 0; x = (2.0*x-one)/(2.0+x);
	    } else {			/* 11/16<=|x|< 19/16 */
		id = 1; x  = (x-one)/(x+one);
	    }
	} else {
	    if (ix < 0x40038000) {	/* |x| < 2.4375 */
		id = 2; x  = (x-1.5)/(one+1.5*x);
	    } else {			/* 2.4375 <= |x| < 2^66 */
		id = 3; x  = -1.0/x;
	    }
	}}
    /* end of argument reduction */
	z = x*x;
	w = z*z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
	s1 = z*(aT[0]+w*(aT[2]+w*(aT[4]+w*(aT[6]+w*(aT[8]+w*aT[10])))));
	s2 = w*(aT[1]+w*(aT[3]+w*(aT[5]+w*(aT[7]+w*aT[9]))));
	if (id<0) return x - x*(s1+s2);
	else {
	    z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	    return (hx<0)? -z:z;
	}
}
