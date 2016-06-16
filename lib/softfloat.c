#include <inttypes.h>

unsigned int __udivmodsi4(unsigned int num, unsigned int den, unsigned int * rem_p)
{
	unsigned int quot = 0, qbit = 1;

	if (den == 0)
	{
		return 0;
	}

	/*
	 * left-justify denominator and count shift
	 */
	while ((signed int) den >= 0)
	{
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit)
	{
		if (den <= num)
		{
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem_p)
		*rem_p = num;

	return quot;
}

unsigned int __aeabi_uidiv(unsigned int num, unsigned int den)
{
	return __udivmodsi4(num, den, 0);
}

/**
 * 这个函数要求返回值是商，寄存器r1中保存余数
 */
unsigned int __aeabi_uidivmod(unsigned int num, unsigned int den)
{
	unsigned int quot = __udivmodsi4(num, den, 0);
	__asm__ __volatile__ ("sub r1, %0, %1\n\t":  : "r" (num), "r" (den*quot) : "r1" );
	return quot;
}

/*============================================================================
This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3a, by John R. Hauser.
Copyright 2011, 2012, 2013, 2014 The Regents of the University of California.
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=============================================================================*/

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;
typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

typedef int bool;
#define false 0
#define true  1

typedef struct { uint32_t v; } float32_t;
typedef struct { uint64_t v; } float64_t;
union ui32_f32 { uint32_t ui; float32_t f; };
union ui64_f64 { uint64_t ui; float64_t f; };
struct exp16_sig64 { int_fast16_t exp; uint_fast64_t sig; };

enum {
    softfloat_round_near_even   = 0,
    softfloat_round_minMag      = 1,
    softfloat_round_min         = 2,
    softfloat_round_max         = 3,
    softfloat_round_near_maxMag = 4
};

enum {
    softfloat_tininess_beforeRounding = 0,
    softfloat_tininess_afterRounding  = 1
};

enum {
    softfloat_flag_inexact   =  1,
    softfloat_flag_underflow =  2,
    softfloat_flag_overflow  =  4,
    softfloat_flag_infinite  =  8,
    softfloat_flag_invalid   = 16
};

uint_fast8_t softfloat_roundingMode = softfloat_round_near_even;
uint_fast8_t softfloat_detectTininess = softfloat_tininess_afterRounding;
uint_fast8_t softfloat_exceptionFlags = 0;
uint_fast8_t extF80_roundingPrecision = 80;

const uint_least8_t softfloat_countLeadingZeros8[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define UINT64_C(v)	v ## ULL
#define defaultNaNF64UI UINT64_C( 0xFFF8000000000000 )

#define signF32UI( a ) ((bool) ((uint32_t) (a)>>31))
#define expF32UI( a ) ((int_fast16_t) ((a)>>23) & 0xFF)
#define fracF32UI( a ) ((a) & 0x007FFFFF)
#define packToF32UI( sign, exp, sig ) (((uint32_t) (sign)<<31) + ((uint32_t) (exp)<<23) + (sig))
#define isNaNF32UI( a ) ((((a) & 0x7F800000) == 0x7F800000) && ((a) & 0x007FFFFF))

#define signF64UI( a ) ((bool) ((uint64_t) (a)>>63))
#define expF64UI( a ) ((int_fast16_t) ((a)>>52) & 0x7FF)
#define fracF64UI( a ) ((a) & UINT64_C( 0x000FFFFFFFFFFFFF ))
#define packToF64UI( sign, exp, sig ) ((uint64_t) (((uint_fast64_t) (sign)<<63) + ((uint_fast64_t) (exp)<<52) + (sig)))
#define isNaNF64UI( a ) ((((a) & UINT64_C( 0x7FF0000000000000 )) == UINT64_C( 0x7FF0000000000000 )) && ((a) & UINT64_C( 0x000FFFFFFFFFFFFF )))

#define softfloat_isSigNaNF64UI( uiA ) ((((uiA) & UINT64_C( 0x7FF8000000000000 )) == UINT64_C( 0x7FF0000000000000 )) && ((uiA) & UINT64_C( 0x0007FFFFFFFFFFFF )))
#define indexWord( total, n ) (n)

void softfloat_raiseFlags( uint_fast8_t flags )
{

}

static uint64_t softfloat_shiftRightJam64( uint64_t a, uint_fast32_t count )
{
    return
        (count < 63) ? a>>count | ((uint64_t) (a<<(-count & 63)) != 0)
            : (a != 0);
}

static void softfloat_mul64To128M( uint64_t a, uint64_t b, uint32_t *zPtr )
{
    uint32_t a32, a0, b32, b0;
    uint64_t z0, mid1, z64, mid;

    a32 = a>>32;
    a0 = a;
    b32 = b>>32;
    b0 = b;
    z0 = (uint64_t) a0 * b0;
    mid1 = (uint64_t) a32 * b0;
    mid = mid1 + (uint64_t) a0 * b32;
    z64 = (uint64_t) a32 * b32;
    z64 += (uint64_t) (mid < mid1)<<32 | mid>>32;
    mid <<= 32;
    z0 += mid;
    zPtr[indexWord( 4, 1 )] = z0>>32;
    zPtr[indexWord( 4, 0 )] = z0;
    z64 += (z0 < mid);
    zPtr[indexWord( 4, 3 )] = z64>>32;
    zPtr[indexWord( 4, 2 )] = z64;
}

static uint_fast8_t softfloat_countLeadingZeros32( uint32_t a )
{
    uint_fast8_t count;

    count = 0;
    if ( a < 0x10000 ) {
        count = 16;
        a <<= 16;
    }
    if ( a < 0x1000000 ) {
        count += 8;
        a <<= 8;
    }
    count += softfloat_countLeadingZeros8[a>>24];
    return count;
}

static uint_fast8_t softfloat_countLeadingZeros64( uint64_t a )
{
    uint_fast8_t count;
    uint32_t a32;

    count = 0;
    a32 = a>>32;
    if ( ! a32 ) {
        count = 32;
        a32 = a;
    }
    /*------------------------------------------------------------------------
    | From here, result is current count + count leading zeros of `a32'.
    *------------------------------------------------------------------------*/
    if ( a32 < 0x10000 ) {
        count += 16;
        a32 <<= 16;
    }
    if ( a32 < 0x1000000 ) {
        count += 8;
        a32 <<= 8;
    }
    count += softfloat_countLeadingZeros8[a32>>24];
    return count;
}

static uint_fast64_t
 softfloat_propagateNaNF64UI( uint_fast64_t uiA, uint_fast64_t uiB )
{
    bool isSigNaNA;

    isSigNaNA = softfloat_isSigNaNF64UI( uiA );
    if ( isSigNaNA || softfloat_isSigNaNF64UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        if ( isSigNaNA ) return uiA | UINT64_C( 0x0008000000000000 );
    }
    return (isNaNF64UI( uiA ) ? uiA : uiB) | UINT64_C( 0x0008000000000000 );
}

static float64_t
 softfloat_roundPackToF64( bool sign, int_fast16_t exp, uint_fast64_t sig )
{
    uint_fast8_t roundingMode;
    bool roundNearEven;
    uint_fast16_t roundIncrement, roundBits;
    bool isTiny;
    uint_fast64_t uiZ;
    union ui64_f64 uZ;

    roundingMode = softfloat_roundingMode;
    roundNearEven = (roundingMode == softfloat_round_near_even);
    roundIncrement = 0x200;
    if ( ! roundNearEven && (roundingMode != softfloat_round_near_maxMag) ) {
        roundIncrement =
            (roundingMode
                 == (sign ? softfloat_round_min : softfloat_round_max))
                ? 0x3FF
                : 0;
    }
    roundBits = sig & 0x3FF;
    if ( 0x7FD <= (uint16_t) exp ) {
        if ( exp < 0 ) {
            isTiny =
                   (softfloat_detectTininess
                        == softfloat_tininess_beforeRounding)
                || (exp < -1)
                || (sig + roundIncrement < UINT64_C( 0x8000000000000000 ));
            sig = softfloat_shiftRightJam64( sig, -exp );
            exp = 0;
            roundBits = sig & 0x3FF;
            if ( isTiny && roundBits ) {
                softfloat_raiseFlags( softfloat_flag_underflow );
            }
        } else if (
            (0x7FD < exp)
                || (UINT64_C( 0x8000000000000000 ) <= sig + roundIncrement)
        ) {
            softfloat_raiseFlags(
                softfloat_flag_overflow | softfloat_flag_inexact );
            uiZ = packToF64UI( sign, 0x7FF, 0 ) - ! roundIncrement;
            goto uiZ;
        }
    }
    if ( roundBits ) softfloat_exceptionFlags |= softfloat_flag_inexact;
    sig = (sig + roundIncrement)>>10;
    sig &= ~(uint_fast64_t) (! (roundBits ^ 0x200) & roundNearEven);
    uiZ = packToF64UI( sign, sig ? exp : 0, sig );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

static float64_t
 softfloat_normRoundPackToF64( bool sign, int_fast16_t exp, uint_fast64_t sig )
{
    int_fast8_t shiftCount;
    union ui64_f64 uZ;

    shiftCount = softfloat_countLeadingZeros64( sig ) - 1;
    exp -= shiftCount;
    if ( (10 <= shiftCount) && ((uint16_t) exp < 0x7FD) ) {
        uZ.ui = packToF64UI( sign, sig ? exp : 0, sig<<(shiftCount - 10) );
        return uZ.f;
    } else {
        return softfloat_roundPackToF64( sign, exp, sig<<shiftCount );
    }
}

static float64_t
 softfloat_addMagsF64( uint_fast64_t uiA, uint_fast64_t uiB, bool signZ )
{
    int_fast16_t expA;
    uint_fast64_t sigA;
    int_fast16_t expB;
    uint_fast64_t sigB;
    int_fast16_t expDiff;
    uint_fast64_t uiZ;
    int_fast16_t expZ;
    uint_fast64_t sigZ;
    union ui64_f64 uZ;

    expA = expF64UI( uiA );
    sigA = fracF64UI( uiA );
    expB = expF64UI( uiB );
    sigB = fracF64UI( uiB );
    expDiff = expA - expB;
    sigA <<= 9;
    sigB <<= 9;
    if ( ! expDiff ) {
        if ( expA == 0x7FF ) {
            if ( sigA | sigB ) goto propagateNaN;
            uiZ = uiA;
            goto uiZ;
        }
        if ( ! expA ) {
            uiZ =
                packToF64UI(
                    signZ, 0, (uiA + uiB) & UINT64_C( 0x7FFFFFFFFFFFFFFF ) );
            goto uiZ;
        }
        expZ = expA;
        sigZ = UINT64_C( 0x4000000000000000 ) + sigA + sigB;
    } else {
        if ( expDiff < 0 ) {
            if ( expB == 0x7FF ) {
                if ( sigB ) goto propagateNaN;
                uiZ = packToF64UI( signZ, 0x7FF, 0 );
                goto uiZ;
            }
            expZ = expB;
            sigA += expA ? UINT64_C( 0x2000000000000000 ) : sigA;
            sigA = softfloat_shiftRightJam64( sigA, -expDiff );
        } else {
            if ( expA == 0x7FF ) {
                if ( sigA ) goto propagateNaN;
                uiZ = uiA;
                goto uiZ;
            }
            expZ = expA;
            sigB += expB ? UINT64_C( 0x2000000000000000 ) : sigB;
            sigB = softfloat_shiftRightJam64( sigB, expDiff );
        }
        sigZ = UINT64_C( 0x2000000000000000 ) + sigA + sigB;
        if ( sigZ < UINT64_C( 0x4000000000000000 ) ) {
            --expZ;
            sigZ <<= 1;
        }
    }
    return softfloat_roundPackToF64( signZ, expZ, sigZ );
 propagateNaN:
    uiZ = softfloat_propagateNaNF64UI( uiA, uiB );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

static float64_t
 softfloat_subMagsF64( uint_fast64_t uiA, uint_fast64_t uiB, bool signZ )
{
    int_fast16_t expA;
    uint_fast64_t sigA;
    int_fast16_t expB;
    uint_fast64_t sigB;
    int_fast16_t expDiff;
    uint_fast64_t uiZ;
    int_fast16_t expZ;
    uint_fast64_t sigZ;
    union ui64_f64 uZ;

    expA = expF64UI( uiA );
    sigA = fracF64UI( uiA );
    expB = expF64UI( uiB );
    sigB = fracF64UI( uiB );
    expDiff = expA - expB;
    sigA <<= 10;
    sigB <<= 10;
    if ( 0 < expDiff ) goto expABigger;
    if ( expDiff < 0 ) goto expBBigger;
    if ( expA == 0x7FF ) {
        if ( sigA | sigB ) goto propagateNaN;
        softfloat_raiseFlags( softfloat_flag_invalid );
        uiZ = defaultNaNF64UI;
        goto uiZ;
    }
    if ( ! expA ) {
        expA = 1;
        expB = 1;
    }
    if ( sigB < sigA ) goto aBigger;
    if ( sigA < sigB ) goto bBigger;
    uiZ = packToF64UI( softfloat_roundingMode == softfloat_round_min, 0, 0 );
    goto uiZ;
 expBBigger:
    if ( expB == 0x7FF ) {
        if ( sigB ) goto propagateNaN;
        uiZ = packToF64UI( signZ ^ 1, 0x7FF, 0 );
        goto uiZ;
    }
    sigA += expA ? UINT64_C( 0x4000000000000000 ) : sigA;
    sigA = softfloat_shiftRightJam64( sigA, -expDiff );
    sigB |= UINT64_C( 0x4000000000000000 );
 bBigger:
    signZ ^= 1;
    expZ = expB;
    sigZ = sigB - sigA;
    goto normRoundPack;
 expABigger:
    if ( expA == 0x7FF ) {
        if ( sigA ) goto propagateNaN;
        uiZ = uiA;
        goto uiZ;
    }
    sigB += expB ? UINT64_C( 0x4000000000000000 ) : sigB;
    sigB = softfloat_shiftRightJam64( sigB, expDiff );
    sigA |= UINT64_C( 0x4000000000000000 );
 aBigger:
    expZ = expA;
    sigZ = sigA - sigB;
 normRoundPack:
    return softfloat_normRoundPackToF64( signZ, expZ - 1, sigZ );
 propagateNaN:
    uiZ = softfloat_propagateNaNF64UI( uiA, uiB );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;

}

static struct exp16_sig64 softfloat_normSubnormalF64Sig( uint_fast64_t sig )
{
    int_fast8_t shiftCount;
    struct exp16_sig64 z;

    shiftCount = softfloat_countLeadingZeros64( sig ) - 11;
    z.exp = 1 - shiftCount;
    z.sig = sig<<shiftCount;
    return z;
}

static float64_t f64_mul( float64_t a, float64_t b )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;
    bool signA;
    int_fast16_t expA;
    uint_fast64_t sigA;
    union ui64_f64 uB;
    uint_fast64_t uiB;
    bool signB;
    int_fast16_t expB;
    uint_fast64_t sigB;
    bool signZ;
    uint_fast64_t magBits;
    struct exp16_sig64 normExpSig;
    int_fast16_t expZ;
#ifdef SOFTFLOAT_FAST_INT64
    struct uint128 sig128Z;
#else
    uint32_t sig128Z[4];
#endif
    uint_fast64_t sigZ, uiZ;
    union ui64_f64 uZ;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA.f = a;
    uiA = uA.ui;
    signA = signF64UI( uiA );
    expA  = expF64UI( uiA );
    sigA  = fracF64UI( uiA );
    uB.f = b;
    uiB = uB.ui;
    signB = signF64UI( uiB );
    expB  = expF64UI( uiB );
    sigB  = fracF64UI( uiB );
    signZ = signA ^ signB;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0x7FF ) {
        if ( sigA || ((expB == 0x7FF) && sigB) ) goto propagateNaN;
        magBits = expB | sigB;
        goto infArg;
    }
    if ( expB == 0x7FF ) {
        if ( sigB ) goto propagateNaN;
        magBits = expA | sigA;
        goto infArg;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( ! expA ) {
        if ( ! sigA ) goto zero;
        normExpSig = softfloat_normSubnormalF64Sig( sigA );
        expA = normExpSig.exp;
        sigA = normExpSig.sig;
    }
    if ( ! expB ) {
        if ( ! sigB ) goto zero;
        normExpSig = softfloat_normSubnormalF64Sig( sigB );
        expB = normExpSig.exp;
        sigB = normExpSig.sig;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expZ = expA + expB - 0x3FF;
    sigA = (sigA | UINT64_C( 0x0010000000000000 ))<<10;
    sigB = (sigB | UINT64_C( 0x0010000000000000 ))<<11;
#ifdef SOFTFLOAT_FAST_INT64
    sig128Z = softfloat_mul64To128( sigA, sigB );
    sigZ = sig128Z.v64 | (sig128Z.v0 != 0);
#else
    softfloat_mul64To128M( sigA, sigB, sig128Z );
    sigZ =
        (uint64_t) sig128Z[indexWord( 4, 3 )]<<32 | sig128Z[indexWord( 4, 2 )];
    if ( sig128Z[indexWord( 4, 1 )] || sig128Z[indexWord( 4, 0 )] ) sigZ |= 1;
#endif
    if ( sigZ < UINT64_C( 0x4000000000000000 ) ) {
        --expZ;
        sigZ <<= 1;
    }
    return softfloat_roundPackToF64( signZ, expZ, sigZ );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 propagateNaN:
    uiZ = softfloat_propagateNaNF64UI( uiA, uiB );
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 infArg:
    if ( ! magBits ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        uiZ = defaultNaNF64UI;
    } else {
        uiZ = packToF64UI( signZ, 0x7FF, 0 );
    }
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 zero:
    uiZ = packToF64UI( signZ, 0, 0 );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

static float64_t f64_sub( float64_t a, float64_t b )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;
    bool signA;
    union ui64_f64 uB;
    uint_fast64_t uiB;
    bool signB;
#if ! defined INLINE_LEVEL || (INLINE_LEVEL < 2)
    float64_t (*magsFuncPtr)( uint_fast64_t, uint_fast64_t, bool );
#endif

    uA.f = a;
    uiA = uA.ui;
    signA = signF64UI( uiA );
    uB.f = b;
    uiB = uB.ui;
    signB = signF64UI( uiB );
#if defined INLINE_LEVEL && (2 <= INLINE_LEVEL)
    if ( signA == signB ) {
        return softfloat_subMagsF64( uiA, uiB, signA );
    } else {
        return softfloat_addMagsF64( uiA, uiB, signA );
    }
#else
    magsFuncPtr =
        (signA == signB) ? softfloat_subMagsF64 : softfloat_addMagsF64;
    return (*magsFuncPtr)( uiA, uiB, signA );
#endif
}

static bool f64_le( float64_t a, float64_t b )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;
    union ui64_f64 uB;
    uint_fast64_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF64UI( uiA ) || isNaNF64UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        return false;
    }
    signA = signF64UI( uiA );
    signB = signF64UI( uiB );
    return
        (signA != signB)
            ? signA || ! ((uiA | uiB) & UINT64_C( 0x7FFFFFFFFFFFFFFF ))
            : (uiA == uiB) || (signA ^ (uiA < uiB));
}

static bool f64_lt( float64_t a, float64_t b )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;
    union ui64_f64 uB;
    uint_fast64_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF64UI( uiA ) || isNaNF64UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        return false;
    }
    signA = signF64UI( uiA );
    signB = signF64UI( uiB );
    return
        (signA != signB)
            ? signA && ((uiA | uiB) & UINT64_C( 0x7FFFFFFFFFFFFFFF ))
            : (uiA != uiB) && (signA ^ (uiA < uiB));
}

static int_fast32_t f64_to_i32_r_minMag( float64_t a, bool exact )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;
    int_fast16_t exp;
    uint_fast64_t sig;
    int_fast16_t shiftCount;
    bool sign;
    int_fast32_t absZ;

    uA.f = a;
    uiA = uA.ui;
    exp = expF64UI( uiA );
    sig = fracF64UI( uiA );
    shiftCount = 0x433 - exp;
    if ( 53 <= shiftCount ) {
        if ( exact && (exp | sig) ) {
            softfloat_exceptionFlags |= softfloat_flag_inexact;
        }
        return 0;
    }
    sign = signF64UI( uiA );
    if ( shiftCount < 22 ) {
        if (
            sign && (exp == 0x41E) && (sig < UINT64_C( 0x0000000000200000 ))
        ) {
            if ( exact && sig ) {
                softfloat_exceptionFlags |= softfloat_flag_inexact;
            }
        } else {
            softfloat_raiseFlags( softfloat_flag_invalid );
            if ( ! sign || ((exp == 0x7FF) && sig) ) return 0x7FFFFFFF;
        }
        return -0x7FFFFFFF - 1;
    }
    sig |= UINT64_C( 0x0010000000000000 );
    absZ = sig>>shiftCount;
    if ( exact && ((uint_fast64_t) (uint_fast32_t) absZ<<shiftCount != sig) ) {
        softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    return sign ? -absZ : absZ;
}

static float64_t i32_to_f64( int32_t a )
{
    uint_fast64_t uiZ;
    bool sign;
    uint_fast32_t absA;
    int_fast8_t shiftCount;
    union ui64_f64 uZ;

    if ( ! a ) {
        uiZ = 0;
    } else {
        sign = (a < 0);
        absA = sign ? -(uint_fast32_t) a : (uint_fast32_t) a;
        shiftCount = softfloat_countLeadingZeros32( absA ) + 21;
        uiZ =
            packToF64UI(
                sign, 0x432 - shiftCount, (uint_fast64_t) absA<<shiftCount );
    }
    uZ.ui = uiZ;
    return uZ.f;
}

/*
 * Copyright (c) 2015, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * On ARM32 EABI defines both a soft-float ABI and a hard-float ABI,
 * hard-float is basically a super set of soft-float. Hard-float requires
 * all the support routines provided for soft-float, but the compiler may
 * choose to optimize to not use some of them.
 *
 * The AEABI functions uses soft-float calling convention even if the
 * functions are compiled for hard-float. So where float and double would
 * have been expected we use aeabi_float_t and aeabi_double_t respectively
 * instead.
 */
typedef unsigned aeabi_float_t;
typedef unsigned long long aeabi_double_t;

/*
 * Helpers to convert between float32 and aeabi_float_t, and float64 and
 * aeabi_double_t used by the AEABI functions below.
 */
// static aeabi_float_t f32_to_f(float32_t val)
// {
	// union {
		// float32_t from;
		// aeabi_float_t to;
	// } res = { .from = val };

	// return res.to;
// }

// static float32_t f32_from_f(aeabi_float_t val)
// {
	// union {
		// aeabi_float_t from;
		// float32_t to;
	// } res = { .from = val };

	// return res.to;
// }

static aeabi_double_t f64_to_d(float64_t val)
{
	union {
		float64_t from;
		aeabi_double_t to;
	} res = { .from = val };

	return res.to;
}

static float64_t f64_from_d(aeabi_double_t val)
{
	union {
		aeabi_double_t from;
		float64_t to;
	} res = { .from = val };

	return res.to;
}

/*
 * From ARM Run-time ABI for ARM Architecture
 * ARM IHI 0043D, current through ABI release 2.09
 *
 * 4.1.2 The floating-point helper functions
 */

/*
 * Table 2, Standard aeabi_double_t precision floating-point arithmetic helper
 * functions
 */

// aeabi_double_t __aeabi_dadd(aeabi_double_t a, aeabi_double_t b)
// {
	// return f64_to_d(f64_add(f64_from_d(a), f64_from_d(b)));
// }

// aeabi_double_t __aeabi_ddiv(aeabi_double_t a, aeabi_double_t b)
// {
	// return f64_to_d(f64_div(f64_from_d(a), f64_from_d(b)));
// }

aeabi_double_t __aeabi_dmul(aeabi_double_t a, aeabi_double_t b)
{
	return f64_to_d(f64_mul(f64_from_d(a), f64_from_d(b)));
}


// aeabi_double_t __aeabi_drsub(aeabi_double_t a, aeabi_double_t b)
// {
	// return f64_to_d(f64_sub(f64_from_d(b), f64_from_d(a)));
// }

aeabi_double_t __aeabi_dsub(aeabi_double_t a, aeabi_double_t b)
{
	return f64_to_d(f64_sub(f64_from_d(a), f64_from_d(b)));
}

/*
 * Table 3, double precision floating-point comparison helper functions
 */

// int __aeabi_dcmpeq(aeabi_double_t a, aeabi_double_t b)
// {
	// return f64_eq(f64_from_d(a), f64_from_d(b));
// }

int __aeabi_dcmplt(aeabi_double_t a, aeabi_double_t b)
{
	return f64_lt(f64_from_d(a), f64_from_d(b));
}

int __aeabi_dcmple(aeabi_double_t a, aeabi_double_t b)
{
	return f64_le(f64_from_d(a), f64_from_d(b));
}

int __aeabi_dcmpge(aeabi_double_t a, aeabi_double_t b)
{
	return f64_le(f64_from_d(b), f64_from_d(a));
}

// int __aeabi_dcmpgt(aeabi_double_t a, aeabi_double_t b)
// {
	// return f64_lt(f64_from_d(b), f64_from_d(a));
// }

/*
 * Table 4, Standard single precision floating-point arithmetic helper
 * functions
 */

// aeabi_float_t __aeabi_fadd(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_to_f(f32_add(f32_from_f(a), f32_from_f(b)));
// }

// aeabi_float_t __aeabi_fdiv(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_to_f(f32_div(f32_from_f(a), f32_from_f(b)));
// }

// aeabi_float_t __aeabi_fmul(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_to_f(f32_mul(f32_from_f(a), f32_from_f(b)));
// }

// aeabi_float_t __aeabi_frsub(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_to_f(f32_sub(f32_from_f(b), f32_from_f(a)));
// }

// aeabi_float_t __aeabi_fsub(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_to_f(f32_sub(f32_from_f(a), f32_from_f(b)));
// }

// /*
 // * Table 5, Standard single precision floating-point comparison helper
 // * functions
 // */

// int __aeabi_fcmpeq(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_eq(f32_from_f(a), f32_from_f(b));
// }

// int __aeabi_fcmplt(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_lt(f32_from_f(a), f32_from_f(b));
// }

// int __aeabi_fcmple(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_le(f32_from_f(a), f32_from_f(b));
// }

// int __aeabi_fcmpge(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_le(f32_from_f(b), f32_from_f(a));
// }

// int __aeabi_fcmpgt(aeabi_float_t a, aeabi_float_t b)
// {
	// return f32_lt(f32_from_f(b), f32_from_f(a));
// }

/*
 * Table 6, Standard floating-point to integer conversions
 */

int __aeabi_d2iz(aeabi_double_t a)
{
	return f64_to_i32_r_minMag(f64_from_d(a), 0);
}

// unsigned __aeabi_d2uiz(aeabi_double_t a)
// {
	// return f64_to_ui32_r_minMag(f64_from_d(a), 0);
// }

// long long __aeabi_d2lz(aeabi_double_t a)
// {
	// return f64_to_i64_r_minMag(f64_from_d(a), 0);
// }

// unsigned long long __aeabi_d2ulz(aeabi_double_t a)
// {
	// return f64_to_ui64_r_minMag(f64_from_d(a), 0);
// }

// int __aeabi_f2iz(aeabi_float_t a)
// {
	// return f32_to_i32_r_minMag(f32_from_f(a), 0);
// }

// unsigned __aeabi_f2uiz(aeabi_float_t a)
// {
	// return f32_to_ui32_r_minMag(f32_from_f(a), 0);
// }

// long long __aeabi_f2lz(aeabi_float_t a)
// {
	// return f32_to_i64_r_minMag(f32_from_f(a), 0);
// }

// unsigned long long __aeabi_f2ulz(aeabi_float_t a)
// {
	// return f32_to_ui64_r_minMag(f32_from_f(a), 0);
// }

/*
 * Table 7, Standard conversions between floating types
 */

// aeabi_float_t __aeabi_d2f(aeabi_double_t a)
// {
	// return f32_to_f(f64_to_f32(f64_from_d(a)));
// }

// aeabi_double_t __aeabi_f2d(aeabi_float_t a)
// {
	// return f64_to_d(f32_to_f64(f32_from_f(a)));
// }

/*
 * Table 8, Standard integer to floating-point conversions
 */

aeabi_double_t __aeabi_i2d(int a)
{
	return f64_to_d(i32_to_f64(a));
}

// aeabi_double_t __aeabi_ui2d(unsigned a)
// {
	// return f64_to_d(ui32_to_f64(a));
// }

// aeabi_double_t __aeabi_l2d(long long a)
// {
	// return f64_to_d(i64_to_f64(a));
// }

// aeabi_double_t __aeabi_ul2d(unsigned long long a)
// {
	// return f64_to_d(ui64_to_f64(a));
// }

// aeabi_float_t __aeabi_i2f(int a)
// {
	// return f32_to_f(i32_to_f32(a));
// }

// aeabi_float_t __aeabi_ui2f(unsigned a)
// {
	// return f32_to_f(ui32_to_f32(a));
// }

// aeabi_float_t __aeabi_l2f(long long a)
// {
	// return f32_to_f(i64_to_f32(a));
// }

// aeabi_float_t __aeabi_ul2f(unsigned long long a)
// {
	// return f32_to_f(ui64_to_f32(a));
// }