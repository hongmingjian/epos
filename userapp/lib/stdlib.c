#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>

void exit(int code)
{
    task_exit(code);
}

/* Copyright (C) 1992, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
ldiv_t
ldiv (long int numer, long int denom)
{
  ldiv_t result;

  result.quot = numer / denom;
  result.rem = numer % denom;

  /* The ANSI standard says that |QUOT| <= |NUMER / DENOM|, where
     NUMER / DENOM is to be computed in infinite precision.  In
     other words, we should always truncate the quotient towards
     zero, never -infinity.  Machine division and remainer may
     work either way when one or both of NUMER or DENOM is
     negative.  If only one is negative and QUOT has been
     truncated towards -infinity, REM will have the same sign as
     DENOM and the opposite sign of NUMER; if both are negative
     and QUOT has been truncated towards -infinity, REM will be
     positive (will have the opposite sign of NUMER).  These are
     considered `wrong'.  If both are NUM and DENOM are positive,
     RESULT will always be positive.  This all boils down to: if
     NUMER >= 0, but REM < 0, we got the wrong answer.  In that
     case, to get the right answer, add 1 to QUOT and subtract
     DENOM from REM.  */

  if (numer >= 0 && result.rem < 0)
    {
      ++result.quot;
      result.rem -= denom;
    }

  return result;
}

div_t
div (numer, denom)
     int numer, denom;
{
  div_t result;

  result.quot = numer / denom;
  result.rem = numer % denom;

  /* The ANSI standard says that |QUOT| <= |NUMER / DENOM|, where
     NUMER / DENOM is to be computed in infinite precision.  In
     other words, we should always truncate the quotient towards
     zero, never -infinity.  Machine division and remainer may
     work either way when one or both of NUMER or DENOM is
     negative.  If only one is negative and QUOT has been
     truncated towards -infinity, REM will have the same sign as
     DENOM and the opposite sign of NUMER; if both are negative
     and QUOT has been truncated towards -infinity, REM will be
     positive (will have the opposite sign of NUMER).  These are
     considered `wrong'.  If both are NUM and DENOM are positive,
     RESULT will always be positive.  This all boils down to: if
     NUMER >= 0, but REM < 0, we got the wrong answer.  In that
     case, to get the right answer, add 1 to QUOT and subtract
     DENOM from REM.  */

  if (numer >= 0 && result.rem < 0)
    {
      ++result.quot;
      result.rem -= denom;
    }

  return result;
}

#include "../lib/tlsf/tlsf.h"
void *malloc(size_t bytes)
{
    return tlsf_malloc(bytes);
}
void *calloc(size_t num, size_t size)
{
    return tlsf_calloc(num, size);
}
void *realloc(void *oldptr, size_t bytes)
{
    return tlsf_realloc(oldptr, bytes);
}
void free(void *ptr)
{
    tlsf_free(ptr);
}

/*-
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Posix rand_r function added May 1999 by Wes Peters <wes@softweyr.com>.
 */
static int
do_rand(unsigned long *ctx)
{
#ifdef  USE_WEAK_SEEDING
/*
 * Historic implementation compatibility.
 * The random sequences do not vary much with the seed,
 * even with overflowing.
 */
    return ((*ctx = *ctx * 1103515245 + 12345) % ((unsigned long)RAND_MAX + 1));
#else   /* !USE_WEAK_SEEDING */
/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * wihout overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
    long hi, lo, x;

    /* Can't be initialized with 0, so use another value. */
    if (*ctx == 0)
        *ctx = 123459876;
    hi = *ctx / 127773;
    lo = *ctx % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    return ((*ctx = x) % ((unsigned long)RAND_MAX + 1));
#endif  /* !USE_WEAK_SEEDING */
}

int
rand_r(unsigned int *ctx)
{
    unsigned long val = (unsigned long) *ctx;
    int r = do_rand(&val);

    *ctx = (unsigned int) val;
    return (r);
}

static unsigned long next = 1;

int
rand()
{
    return (do_rand(&next));
}

void
srand(unsigned int seed)
{
    next = seed;
}