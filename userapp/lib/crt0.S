  .extern ___main
  .extern _main

  .globl _start
_start:
#ifdef __ELF__
  push {r0}
  bl ___main
  pop {r0}
#endif
  b _main
