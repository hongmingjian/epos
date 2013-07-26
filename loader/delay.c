/*
 * delay.c:  Delay calibration, the Linux way
 *
 * By Ramon van Handel (19/12/98)
 * Based on the method used in linux 2.0.30 init/main.c
 *
 * Resembles a lot the way I thought up in my original email, just a tiny
 * bit more accurate.  The principle is the same, implementation is
 * different.  I didn't make this as detailed as linux does, but I
 * documented it pretty well.  If you really want to trim this to the
 * microsecond, have a look at the linux sources.  The main calibration
 * code (init/main.c) is similar, but have a look in include/linux/delay.h,
 * include/asm-i386/delay.h, and arch/i386/lib/delay.S.
 *                                                        -- Ramon
 */
/*
GazOS Operating System
Copyright (C) 1999  Gareth Owen <gaz@athene.co.uk>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "libepc.h"

/**************************************************************************/

/*
 * Allright, let's review how we're going to do this.
 *
 * Goal:
 * The goal is to create a reasonably accurate version of the delay()
 * function, which creates a delay in multiples of one millisecond.
 *
 * Restrictions:
 * We don't want to use the PIT for the delay.  In your average
 * bare-hardware system (ie OS) you'll be using the PIT for quite a few
 * other things already (in my OS code, channel 0 and 1 are used by the
 * scheduler, and channel 2 would probably be needed to control the
 * speaker.)
 *
 * Method:
 * A simple loop would delay the computer wonderfully.  Ie, take something
 * like:
 *
 *         for(i=0;i<BIGNUMBER;i++);  // Delay loop
 *
 * That kills time wonderfully.  All we need to do is to find out what
 * BIGNUMBER we need to use to delay one millisecond.  Finding the correct
 * BIGNUMBER for your machine is called *delay loop calibration*.
 *
 * We can calibrate the delay loop using the PIT.  In the initialisation
 * phase of the OS, when the PIT is not yet hooked to the scheduler, it
 * can be freely used to do this.  When the delay loop has been calibrated
 * against the PIT, the PIT is free to be used for other purposes.
 *
 * There are many ways to calibrate a delay loop with the PIT.  This is
 * one way (used in Linux.)  More documentation in the code.
 */

/**************************************************************************/

/*
 * The delay function:
 * Have a look at this function before you look at the calibration code.
 * Basically, delay_count is the number you have to count to in order to
 * kill 1ms.  delay() takes the amount of milliseconds to delay as
 * parameter.
 *
 * The goal of the rest of this code is to determine what delay_count *is*
 * for your machine.
 *
 * Keep delay_count initialised to 1.  We'll need that later.
 */

static unsigned long delay_count = 1;

/*
 * When I was testing the delay code, I notice that the calibration we use
 * is very delicate.  Originally, I had inlined the delay for() loops into
 * the delay() and calibrateDelayLoop() functions, but this doesn't work.
 * Due to different register allocation or something similar the compiler
 * might have generated different code in the two situations (I didn't
 * check this.)  Or alignment differences might have caused the problems.
 * Anyway, to solve the problem, we always use the __delay() function for
 * the delay for() loop, because it always is the same code with the same
 * alignment. __delay() is called from delay() as well as from
 * calibrateDelayLoop().  By using __delay() we can fine-tune our
 * calibration without it losing its finesse afterwards.
 */

static void __delay(unsigned long loops)
{
    unsigned long c;
    for(c=0;c<loops;c++);
}

void delay(unsigned long milliseconds)
{
    __delay(milliseconds*delay_count);     /* Delay milliseconds ms */
}

/**************************************************************************/

/*
 * The timer ISR:
 * In order to calibrate the delay loop, we program the PIT channel 0 with
 * a fixed timer rate of MILLISEC milliseconds.  The only thing the ISR
 * does is increment a timer, ticks, on every interrupt (that is, every
 * 10ms in this case.)  The main program will use this counter in order to
 * calibrate the delay loop.
 */

#define HZ 100
#define MILLISEC (1000/HZ)           /* Take a 10ms fixed timer rate   */

static volatile unsigned long ticks = 0;

static void delayCalibInt(int irq)      /* The timer ISR                  */
{
    ticks++;                  /* Increment the ticks counter    */
}

/**************************************************************************/

/*
 * Calibrate the delay loop:
 * This is the most important part of the whole business.  I'll explain
 * the general strategy here, which is supplemented by comments in the
 * code.
 *
 * Stage 1:
 * First, we are going to make a rough approximation of delay_count. We
 * are going to try to determine between which two powers of 2 delay_count
 * needs to be.  In order to do that, we first wait until the next timer
 * tick has started (we know it has when the ticks variable has just been
 * incremented.)  Then we start a delay with delay_count of 1.  When the
 * delay has finished, we look whether a timer tick has passed during the
 * delay (nah !).  If not, we shift delay_count to the right once and try
 * again.  We continue until after the delay, one timer tick has passed.
 * We then know that our value for delay_count is too big, so the 'real'
 * delay_count is between (delay_count) and (delay_count>>1).
 *
 * Stage 2:
 * Afterwards, we start refining our value.  We shift our delay_count to
 * the right once to obtain out bottom-value for delay_count (ie we then
 * know that the 'real' delay_count is bigger than the actual
 * delay_count.)  delay_count is a power of 2, so only one bit is set.
 * What we do is, we start setting bits in delay_count and see whether
 * delay_count is bigger than a tick afterwards. An example helps
 * clarifying matters:
 *
 * Imagine we found in stage 1 that delay_count is between 00100000b and
 * 01000000b.  (In reality it would be much bigger, but for the example
 * this will do.)  Before we start our fine-calibration, we set
 * delay_count to 00100000b.
 * Now we turn on the bit after the set bit, ie:  00110000b
 * We do the delay like we did previously, and see whether ticks has
 * changed during it.  If it has, we know that the value is too big, and
 * turn the bit back off.  If it hasn't, we leave it on.  In either case,
 * we proceed with the next bit:
 *
 *     00100000b   <--- start value
 *     00110000b   <--- tick hasn't passed
 *     00111000b   <--- tick has passed, turn it off: 00110000b
 *     00110100b   <--- tick has passed, turn it off: 00110000b
 *     00110010b   <--- tick hasn't passed
 *     00110011b   <--- tick hasn't passed
 *
 * And there is our delay_count value.
 *
 * In order to save time (calibrating can be a time-consuming matter, and
 * tuning it extremely finely is often useless because at a certain level
 * you get additional delay by calling the delay() function, and returning
 * from it, etc.) we only calculate delay_count up to a certain precision
 * (ie, we only calculate PRECISION extra bits after stage 1.)  A
 * PRECISION value of 8 gives good results (according to the linux
 * sources, it gives an imprecision of approximately 1%.)
 */

#define PRECISION 8                 /* Calibration precision          */

unsigned long init_delay(void)
{
    unsigned int prevtick;          /* Temporary variable             */
    unsigned int i;                 /* Counter variable               */
    unsigned int calib_bit;         /* Bit to calibrate (see below)   */

    void (*pfnOld)(int irq)=intr_vector[IRQ_TIMER]; // Save ISR

    intr_vector[IRQ_TIMER] = delayCalibInt;  // Install ISR 
    outportb(0x21, inportb(0x21) & ~0x01) ; // Enable the interrupt

    /* Stage 1:  Coarse calibration                                   */

    do {
        delay_count <<= 1;          /* Next delay count to try        */

        prevtick=ticks;             /* Wait for the start of the next */
        while(prevtick==ticks);     /* timer tick                     */

        prevtick=ticks;             /* Start measurement now          */
        __delay(delay_count);       /* Do the delay                   */
    } while(prevtick == ticks);     /* Until delay is just too big    */

    delay_count >>= 1;              /* Get bottom value for delay     */

    /* Stage 2:  Fine calibration                                     */

    calib_bit = delay_count;        /* Which bit are we going to test */

    for(i=0;i<PRECISION;i++) {
        calib_bit >>= 1;            /* Next bit to calibrate          */
        if(!calib_bit) break;       /* If we have done all bits, stop */

        delay_count |= calib_bit;   /* Set the bit in delay_count     */

        prevtick=ticks;             /* Wait for the start of the next */
        while(prevtick==ticks);     /* timer tick                     */

        prevtick=ticks;             /* Start measurement now          */
        __delay(delay_count);       /* Do the delay                   */

        if(prevtick != ticks)       /* If a tick has passed, turn the */
            delay_count &= ~calib_bit;     /* calibrated bit back off */
    }

    /* We're finished:  Do the finishing touches                      */

    intr_vector[IRQ_TIMER] = pfnOld;  // Restore ISR 
    outportb(0x21, inportb(0x21) | 0x01) ; // Disable the interrupt

    delay_count /= MILLISEC;       /* Calculate delay_count for 1ms   */

    return delay_count;
}

/**************************************************************************/

/* The end */
