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
#include "kernel.h"
#include "utils.h"

unsigned volatile g_timer_ticks = 0;

void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    //sys_putchar('.');

    if(g_task_running != NULL) {
        if(g_task_running->tid == 0) {
            g_resched = 1;
        } else {
            --g_task_running->timeslice;

            if(g_task_running->timeslice <= 0) {
                g_resched = 1;
                g_task_running->timeslice = DEFAULT_TIMESLICE;
            }
        }
    }
}
