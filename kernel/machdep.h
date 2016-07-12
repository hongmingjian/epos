/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _MACHDEP_H
#define _MACHDEP_H

#include <sys/types.h>
#include "board.h"

#define CF_SPSR     0
#define CF_R0       4
#define CF_R1       8
#define CF_R2       12
#define CF_R3       16
#define CF_R4       20
#define CF_R5       24
#define CF_R6       28
#define CF_R7       32
#define CF_R8       36
#define CF_R9       40
#define CF_R10      44
#define CF_R11      48
#define CF_R12      52
#define CF_USR_SP   56
#define CF_USR_LR   60
#define CF_SVC_SP   64
#define CF_SVC_LR   68
#define CF_PC       72

#define FRAME_SIZE  76

struct context {
    uint32_t cf_spsr;
    uint32_t cf_r0;
    uint32_t cf_r1;
    uint32_t cf_r2;
    uint32_t cf_r3;
    uint32_t cf_r4;
    uint32_t cf_r5;
    uint32_t cf_r6;
    uint32_t cf_r7;
    uint32_t cf_r8;
    uint32_t cf_r9;
    uint32_t cf_r10;
    uint32_t cf_r11;
    uint32_t cf_r12;
    uint32_t cf_usr_sp;
    uint32_t cf_usr_lr;
    uint32_t cf_svc_sp;
    uint32_t cf_svc_lr;
    uint32_t cf_pc;
};

#define STACK_PUSH(sp, value) do { \
    (sp)-=4; \
    *((uint32_t *)(sp)) = (uint32_t)(value); \
} while(0)

#define INIT_TASK_CONTEXT(ustack, kstack, entry, pv) do { \
    STACK_PUSH(kstack, entry);\
    STACK_PUSH(kstack, 0); \
    STACK_PUSH(kstack, 0); \
    STACK_PUSH(kstack, 0); \
    STACK_PUSH(kstack, ustack); \
    STACK_PUSH(kstack, 0xCCCCCCCC); \
    STACK_PUSH(kstack, 0xBBBBBBBB); \
    STACK_PUSH(kstack, 0xAAAAAAAA); \
    STACK_PUSH(kstack, 0x99999999); \
    STACK_PUSH(kstack, 0x88888888); \
    STACK_PUSH(kstack, 0x77777777); \
    STACK_PUSH(kstack, 0x66666666); \
    STACK_PUSH(kstack, 0x55555555); \
    STACK_PUSH(kstack, 0x44444444); \
    STACK_PUSH(kstack, 0x33333333); \
    STACK_PUSH(kstack, 0x22222222); \
    STACK_PUSH(kstack, 0x11111111); \
    STACK_PUSH(kstack, pv); \
    STACK_PUSH(kstack, (ustack)?0x50:0x53); \
    STACK_PUSH(kstack, &ret_from_syscall); \
} while(0)

#define run_as_task0() do { \
    g_task_running = task0; \
    __asm__ __volatile__ ( \
            "ldr r0, %0\n\t" \
            "ldr r0, [r0]\n\t" \
            "ldr r1, =1f\n\t" \
            "str r1, [r0, #76]\n\t" \
            "mov sp, r0\n\t" \
            "ldmia sp!, {pc}\n\t" \
            "1:\n\t" \
            : \
            : "m"(g_task_running) \
            : "r0", "r1");\
} while(0)

void sti(), cli();
void isr_timer(uint32_t irq, struct context *ctx);

#endif /*_MACHDEP_H*/
