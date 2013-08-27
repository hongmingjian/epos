#include "../syscall.h"

  .globl _task_exit
_task_exit:
  movl $SYSCALL_TASK_EXIT, %eax
  pushl 4(%esp)
  int $0x77
  addl $4, %esp
  ret

  .globl _task_create
_task_create:
  movl $SYSCALL_TASK_CREATE, %eax
  pushl 12(%esp)
  pushl 12(%esp)
  pushl 12(%esp)
  int $0x77
  addl $12, %esp
  ret

  .globl _task_getid
_task_getid:
  movl $SYSCALL_TASK_GETID, %eax
  int $0x77
  ret

 .globl _task_yield
_task_yield:
  movl $SYSCALL_TASK_YIELD, %eax
  int $0x77
  ret

 .globl _task_wait
_task_wait:
  movl $SYSCALL_TASK_WAIT, %eax
  pushl 8(%esp)
  pushl 8(%esp)
  int $0x77
  addl $8, %esp
  ret

  .globl _task_sleep
_task_sleep:
  movl $SYSCALL_TASK_SLEEP, %eax
  pushl 4(%esp)
  int $0x80
  addl $4, %esp
  ret

  .globl _putchar
_putchar:
  movl $SYSCALL_PUTCHAR, %eax
  pushl 4(%esp)
  int $0x77
  addl $4, %esp
  ret

