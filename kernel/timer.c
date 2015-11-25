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
                g_task_running->timeslice = DEFAULT_TIMESLICE;
            }
        }
    }
}
