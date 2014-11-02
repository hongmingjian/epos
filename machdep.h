/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
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

#include "cpu.h"

/**
 * `struct segment_descriptor' comes from FreeBSD
 */
#define SEL_KPL 0  /* kernel priority level */
#define SEL_UPL 3  /*   user priority level */

#define GSEL_NULL   0 /*        Null Descriptor */
#define GSEL_KCODE  1 /* Kernel Code Descriptor */
#define GSEL_KDATA  2 /* Kernel Data Descriptor */
#define GSEL_UCODE  3 /*   User Code Descriptor */
#define GSEL_UDATA  4 /*   User Data Descriptor */
#define GSEL_TSS    5 /*  Common TSS Descriptor */
#define NR_GDT      6
struct segment_descriptor {
  unsigned lolimit:16 ;
  unsigned lobase:24 __attribute__ ((packed));
  unsigned type:5 ;
  unsigned dpl:2 ;
  unsigned p:1 ;
  unsigned hilimit:4 ;
  unsigned xx:2 ;
  unsigned def32:1 ;
  unsigned gran:1 ;
  unsigned hibase:8 ;
};

struct x87 {
  uint32_t cwd;
  uint32_t swd;
  uint32_t twd;
  uint32_t fip;
  uint32_t fcs;
  uint32_t foo;
  uint32_t fos;
  uint8_t  st[80];
};

#define IRQ_TIMER     0
#define IRQ_KEYBOARD  1
#define IRQ_FDC       6
#define NR_IRQ        16

struct context {
  uint32_t   es;       /* 0*/
  uint32_t   ds;       /* 4*/
  uint32_t   fs;       /* 8*/
  uint32_t  edi;       /*12*/
  uint32_t  esi;       /*16*/
  uint32_t  ebp;       /*20*/
  uint32_t  isp;       /*24*/
  uint32_t  ebx;       /*28*/
  uint32_t  edx;       /*32*/
  uint32_t  ecx;       /*36*/
  uint32_t  eax;       /*40*/
  uint32_t  exception; /*44*/
  uint32_t  errorcode; /*48*/
  /* below defined in x86 hardware */
  uint32_t  eip;       /*52*/
  uint32_t   cs;       /*56*/
  uint32_t  eflags;    /*60*/
  /* below only when crossing rings */
  uint32_t  esp;       /*64*/
  uint32_t   ss;       /*68*/
};

#define STACK_PUSH(sp, value) do { \
  (sp)-=4; \
  *((uint32_t *)(sp)) = (uint32_t)(value); \
} while(0)

#define INIT_TASK_CONTEXT(ustack, kstack, entry) do { \
  uint32_t n = sizeof(struct segment_descriptor); \
  if(ustack) { \
    STACK_PUSH(kstack, (GSEL_UDATA*n)|SEL_UPL); \
    STACK_PUSH(kstack, ustack); \
  } \
  STACK_PUSH(kstack, 0x202); \
  STACK_PUSH(kstack, \
    (ustack)?((GSEL_UCODE*n)|SEL_UPL):((GSEL_KCODE*n)|SEL_KPL)); \
  STACK_PUSH(kstack, entry); \
  STACK_PUSH(kstack, 0xdeadbeef); \
  STACK_PUSH(kstack, 0xbeefdead); \
  STACK_PUSH(kstack, 0x11111111); \
  STACK_PUSH(kstack, 0x22222222); \
  STACK_PUSH(kstack, 0x33333333); \
  STACK_PUSH(kstack, 0x44444444); \
  STACK_PUSH(kstack, 0x55555555); \
  STACK_PUSH(kstack, 0x66666666); \
  STACK_PUSH(kstack, 0x77777777); \
  STACK_PUSH(kstack, 0x88888888); \
  STACK_PUSH(kstack, \
    (ustack)?((GSEL_UDATA*n)|SEL_UPL):((GSEL_KDATA*n)|SEL_KPL)); \
  STACK_PUSH(kstack, \
    (ustack)?((GSEL_UDATA*n)|SEL_UPL):((GSEL_KDATA*n)|SEL_KPL)); \
  STACK_PUSH(kstack, \
    (ustack)?((GSEL_UDATA*n)|SEL_UPL):((GSEL_KDATA*n)|SEL_KPL)); \
  STACK_PUSH(kstack, &ret_from_syscall); \
} while(0)

#define run_as_task0() do { \
  g_task_running = task0; \
  __asm__ __volatile__ ( \
    "movl %0, %%eax\n\t" \
    "movl (%%eax), %%eax\n\t" \
    "pushl $1f\n\t" \
    "popl (52+4)(%%eax)\n\t" \
    "movl %%eax, %%esp\n\t" \
    "ret\n\t" \
    "1:\n\t" \
    : \
    :"m"(g_task_running) \
    :"%eax" \
  ); \
} while(0)

void init_machdep(uint32_t mbi, uint32_t physfree);
void sys_beep(int32_t freq);

struct vm86_context {
  uint32_t  : 32;/*0*/
  uint32_t  : 32;/*4*/
  uint32_t  : 32;/*8*/
  uint32_t  edi; /*12*/
  uint32_t  esi; /*16*/
  uint32_t  ebp; /*20*/
  uint32_t  : 32;/*24*/
  uint32_t  ebx; /*28*/
  uint32_t  edx; /*32*/
  uint32_t  ecx; /*36*/
  uint32_t  eax; /*40*/
  uint32_t  : 32;/*44*/
  uint32_t  : 32;/*48*/
  /* below defined in x86 hardware */
  uint32_t  eip; /*52*/
  uint16_t  cs; uint16_t  : 16;/*56*/
  uint32_t  eflags;/*60*/
  uint32_t  esp; /*64*/
  uint16_t  ss; uint16_t  : 16;/*68*/
  uint16_t  es; uint16_t  : 16;/*72*/
  uint16_t  ds; uint16_t  : 16;/*76*/
  uint16_t  fs; uint16_t  : 16;/*80*/
  uint16_t  gs; uint16_t  : 16;/*84*/
};
void sys_vm86(struct vm86_context *vm86ctx);

#endif /*_MACHDEP_H*/
