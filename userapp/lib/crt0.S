  .extern ___main
  .extern _main

  .globl _start
_start:
#ifdef __ELF__
  call ___main
#endif
  jmp _main

