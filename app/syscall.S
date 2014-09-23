#include "../syscall.h"

#define STUB(name) \
  .globl _ ## name; \
_ ## name: \
    movl $SYSCALL_ ## name, %eax; \
    int $0x82; \
    ret

STUB(task_exit)
STUB(task_create)
STUB(task_getid)
STUB(task_yield)
STUB(task_wait)
STUB(beep)
STUB(vm86)
STUB(putchar)
