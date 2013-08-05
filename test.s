  .globl _start 
_start:
  call _putc
  jmp _start

  .globl _putc
_putc:
  pushl $'s'
  xorl %eax, %eax
  int $0x80
  addl $4, %esp
  ret
