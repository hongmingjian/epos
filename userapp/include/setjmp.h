#ifndef _SETJMP_H
#define _SETJMP_H

#include <stdint.h>

typedef struct {
  uint32_t ebx;
  uint32_t esi;
  uint32_t edi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t eip;
} jmp_buf[1];


int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int value);

#endif /*_SETJMP_H*/
