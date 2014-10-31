/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 *
 * Copyright (C) 2014 Hong MingJian<hongmingjian@gmail.com>
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
#include "../global.h"
#include "syscall.h"

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))

int vm86call(int fintr, uint32_t n, struct vm86_context *vm86ctx)
{
  int fStop = 0, res = 0;
  uint16_t ip, sp;
  uint16_t ret_cs = 0, ret_ip = 0x600;/*XXX*/
  *(uint8_t *)LADDR(ret_cs, ret_ip) = 0xf4/*HLT*/;

  sp = LOWORD(vm86ctx->esp);
  if(fintr)
    sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = 0;
  sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = ret_cs;
  sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = ret_ip;

  if(fintr) {
    uint16_t *ivt = (uint16_t *)0;

	n = n & 0xff;
	ip = ivt[n * 2 + 0];
	vm86ctx->cs = ivt[n * 2 + 1];
  } else {
    ip = LOWORD(n);
    vm86ctx->cs = HIWORD(n);
  }

  vm86ctx->eip = (vm86ctx->eip & 0xffff0000) | ip;
  vm86ctx->esp = (vm86ctx->esp & 0xffff0000) | sp;
  do {
    vm86(vm86ctx);

    ip = LOWORD(vm86ctx->eip);
    sp = LOWORD(vm86ctx->esp);

    switch(*(uint8_t *)LADDR(vm86ctx->cs, ip)) {
    case 0xf4: /*HLT*/
      if(LADDR(vm86ctx->cs, ip)==LADDR(ret_cs, ret_ip))
        fStop = 1;
      ip++;
      break;
    default:
      fStop = 1;
      printf("Unknown opcode: 0x%02x at 0x%04x:0x%04x\r\n",
             *(uint8_t *)LADDR(vm86ctx->cs, ip),
	     vm86ctx->cs, ip);
      res = -1;
      break;
    }
    vm86ctx->eip = (vm86ctx->eip & 0xffff0000) | ip;
    vm86ctx->esp = (vm86ctx->esp & 0xffff0000) | sp;
  } while(!fStop);

  return res;
}
