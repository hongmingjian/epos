  .globl _putchar
_putchar:
  pushl 4(%esp)
  xorl %eax, %eax
  int $0x80
  addl $4, %esp
  ret

  .globl _task_getid
_task_getid:
  movl $1, %eax
  int $0x80
  ret

  .globl _task_yield
_task_yield:
  movl $2, %eax
  int $0x80
  ret

  .globl _task_sleep
_task_sleep:
  movl $3, %eax
  pushl 4(%esp)
  int $0x80
  addl $4, %esp
  ret

  .globl _task_create
_task_create:
  movl $4, %eax
  pushl 12(%esp)
  pushl 12(%esp)
  pushl 12(%esp)
  int $0x80
  addl $12, %esp
  ret

  .globl _task_exit
_task_exit:
  movl $5, %eax
  pushl 4(%esp)
  int $0x80
  addl $4, %esp
  ret

  .globl _task_wait
_task_wait:
  movl $6, %eax
  pushl 8(%esp)
  pushl 8(%esp)
  int $0x80
  addl $8, %esp
  ret

