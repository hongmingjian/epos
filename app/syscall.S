#include "../syscall.h"

  .globl _task_exit
_task_exit:
  movl $SYSCALL_TASK_EXIT, %eax
  int $0x82
  ret

  .globl _task_create
_task_create:
  movl $SYSCALL_TASK_CREATE, %eax
  int $0x82
  ret

  .globl _task_getid
_task_getid:
  movl $SYSCALL_TASK_GETID, %eax
  int $0x82
  ret

 .globl _task_yield
_task_yield:
  movl $SYSCALL_TASK_YIELD, %eax
  int $0x82
  ret

 .globl _task_wait
_task_wait:
  movl $SYSCALL_TASK_WAIT, %eax
  int $0x82
  ret

 .globl _beep
_beep:
  movl $SYSCALL_BEEP, %eax
  int $0x82
  ret

  .globl _putchar
_putchar:
  movl $SYSCALL_PUTCHAR, %eax
  int $0x82
  ret
