/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
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
#include <stddef.h>
#include <time.h>
#include "kernel.h"

/*记录系统启动以来，定时器中断的次数*/
unsigned volatile g_timer_ticks = 0;

/**
 * 定时器的中断处理程序
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    //sys_putchar('.');

    if(g_task_running != NULL) {
        //如果是task0在运行，则强制调度
        if(g_task_running->tid == 0) {
            g_resched = 1;
        } else {
            //否则，把当前线程的时间片减一
            --g_task_running->timeslice;

            //如果当前线程用完了时间片，也要强制调度
            if(g_task_running->timeslice <= 0) {
                g_resched = 1;
                g_task_running->timeslice = TASK_TIMESLICE_DEFAULT;
            }
        }
    }
}

/**
 * The following code comes from pintos.
 *
 * With some modifications by Mingjian Hong
 */
#define barrier() __asm__ __volatile__ ("" : : : "memory")
static void busy_wait(unsigned loops)
{
  do {
    barrier ();
  } while (loops-- > 0);
}

/* Number of loops per timer tick.
   Initialized by calibrate_delay(). */
static unsigned loops_per_tick;

/* This is the number of bits of precision for the loops_per_tick.  Each
 * bit takes on average 1.5/HZ seconds.  This (like the original) is a little
 * better than 1% */
#define LPS_PREC 8

void calibrate_delay(void)
{
    unsigned long tmp, loopbit;
    int lps_precision = LPS_PREC;

    loops_per_tick = (1<<12);

    printk("Calibrating delay... ");

    while (loops_per_tick <<= 1) {
        /* wait for "start of" clock tick */
        tmp = g_timer_ticks;
        while (tmp == g_timer_ticks)
            /* nothing */;
        /* Go .. */
        tmp = g_timer_ticks;
        busy_wait(loops_per_tick);

        tmp = g_timer_ticks - tmp;
        if (tmp)
            break;
    }

    /* Do a binary approximation to get loops_per_tick set to equal one clock
     * (up to lps_precision bits)
     */
    loops_per_tick >>= 1;
    loopbit = loops_per_tick;
    while ( lps_precision-- && (loopbit >>= 1) ) {
        loops_per_tick |= loopbit;

        /* wait for "start of" clock tick */
        tmp = g_timer_ticks;
        while (tmp == g_timer_ticks)
            /* nothing */;
        /* Go .. */
        tmp = g_timer_ticks;
        busy_wait(loops_per_tick);

        if (g_timer_ticks != tmp)   /* longer than 1 tick */
            loops_per_tick &= ~loopbit;
    }

    printk ("%u loops per second (%lu.%02lu BogoMIPS)\r\n",
            loops_per_tick * HZ,
            loops_per_tick/(500000/HZ),
            loops_per_tick/(5000/HZ) % 100);
}

static unsigned _sleep(unsigned ticks)
{
  unsigned start = g_timer_ticks;

  while (g_timer_ticks - start < ticks)
    sys_task_yield ();

  return 0;
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void _delay (unsigned num, unsigned denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  busy_wait (loops_per_tick / (denom / 1000) * (num / 1000) * HZ );
}

static void do_sleep (unsigned num, unsigned denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * HZ / DENOM ticks.
        1 s / HZ ticks
  */
  unsigned ticks = num * HZ / denom;

  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         sys_sleep() because it will yield the CPU to other
         processes. */
      _sleep (ticks);
    }
  else
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      _delay (num, denom);
    }
}

unsigned sys_sleep(unsigned seconds)
{
    do_sleep(seconds, 1);
    return 0;
}

int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    if(rqtp->tv_sec > 0)
        do_sleep(rqtp->tv_sec, 1);

    long nsec = rqtp->tv_nsec,
         nhz  = 1000*1000*1000 / HZ,
         ticks= nsec / nhz;
    if(ticks > 0) {
        do_sleep(ticks, HZ);
        nsec %= nhz;
    }
    if(nsec > 0)
        do_sleep(nsec, 1000 * 1000 * 1000);

    return 0;
}

int sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if(tv == NULL)
        return -1;
    tv->tv_sec = g_startup_time + g_timer_ticks/HZ;
    tv->tv_usec = (g_timer_ticks%HZ) * 1000;
    return 0;
}
