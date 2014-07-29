/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
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

unsigned volatile ticks = 0;

void isr_timer(uint32_t irq, struct context *ctx)
{
  ticks++;
//  sys_putchar('.');

  if(g_task_running != NULL) {
    if(g_task_running->tid == 0) {
      g_resched = 1;
    } else {
      --g_task_running->quantum;

      if(g_task_running->quantum <= 0) {
        g_resched = 1;
        g_task_running->quantum = DEFAULT_QUANTUM;
      }
    }
  }
}
