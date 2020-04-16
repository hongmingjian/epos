#include "syscall-nr.h"

#define WRAPPER(name) \
  .globl _ ## name; \
_ ## name: \
    push {r4}; \
    mov r4, #SYSCALL_ ## name; \
    swi #0; \
    pop {r4}; \
    bx lr

WRAPPER(task_exit)
WRAPPER(task_create)
WRAPPER(task_getid)
WRAPPER(task_yield)
WRAPPER(task_wait)

WRAPPER(mmap)
WRAPPER(munmap)
WRAPPER(sleep)
WRAPPER(nanosleep)
WRAPPER(gettimeofday)

WRAPPER(open)
WRAPPER(close)
WRAPPER(read)
WRAPPER(write)
WRAPPER(lseek)