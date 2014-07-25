#include "../global.h"
#include "syscall.h"

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))

int vm86int(int n, struct vm86_context *vm86ctx)
{
  int fStop = 0, res = 0;
  uint16_t *ivt = (uint16_t *)0;
  uint16_t ip, sp;
  uint16_t iret_cs = 0, iret_ip = 0x500;/*XXX*/

  ip = LOWORD(vm86ctx->eip);
  sp = LOWORD(vm86ctx->esp);

  *(uint8_t *)LADDR(iret_cs, iret_ip) = 0xf4/*HLT*/;

  sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = 0;
  sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = iret_cs;
  sp -= 2; *(uint16_t *)LADDR(vm86ctx->ss, sp) = iret_ip;

  ip = ivt[n * 2 + 0];
  vm86ctx->cs = ivt[n * 2 + 1];

  vm86ctx->eip = (vm86ctx->eip & 0xffff0000) | ip;
  vm86ctx->esp = (vm86ctx->esp & 0xffff0000) | sp;
  do {
    vm86(vm86ctx);

    ip = LOWORD(vm86ctx->eip);
    sp = LOWORD(vm86ctx->esp);

    switch(*(uint8_t *)LADDR(vm86ctx->cs, ip)) {
    case 0xf4: /*HLT*/
      if(LADDR(vm86ctx->cs, ip)==LADDR(iret_cs, iret_ip))
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
