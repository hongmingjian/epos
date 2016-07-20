  .extern _main

  .globl _start
_start:
#ifdef __ELF__
  bl ___main
#endif
  b _main

