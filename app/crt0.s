  .extern _main

  .globl _start
_start:
  jmp _main
  jmp _start

  .globl _putc
_putc:
  pushl 4(%esp)
  xorl %eax, %eax
  int $0x80
  addl $4, %esp
  ret
