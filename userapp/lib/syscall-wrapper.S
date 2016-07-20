#include "syscall-nr.h"

#define WRAPPER(name) \
  .globl _ ## name; \
_ ## name: \
    push {r7}; \
    mov r7, #SYSCALL_ ## name; \
    swi #0; \
    pop {r7}; \
    bx lr

WRAPPER(putchar)
