/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include "../global.h"
#include "math.h"

/*
 * http://en.wikipedia.org/wiki/Multiply-with-carry
 */
#define PHI 0x9e3779b9
static uint32_t Q[4096], c = 362436;
void srand(uint32_t seed)
{
    int i;

    Q[0] = seed;
    Q[1] = seed + PHI;
    Q[2] = seed + PHI + PHI;

    for (i = 3; i < 4096; i++)
        Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;
}
uint32_t random(void)
{
    uint64_t t, a = 18782LL;
    static uint32_t i = 4095;
    uint32_t x, r = 0xfffffffe;
    i = (i + 1) & 4095;
    t = a * Q[i] + c;
    c = (t >> 32);
    x = t + c;
    if (x < c) {
        x++;
        c++;
    }
    return (Q[i] = r - x);
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

