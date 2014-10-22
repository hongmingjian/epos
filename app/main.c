/*
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 */
#include "../global.h"
#include "syscall.h"
#include "math.h"
#include "graphics.h"

///////////////////HELPERS///////////////////////
#include "../tlsf/tlsf.h"
extern char end[];
void *malloc(size_t bytes)
{ return malloc_ex(bytes, end); }
void *realloc(void *oldptr, size_t bytes)
{ return realloc_ex(oldptr, bytes, end); }
void free(void *ptr)
{ free_ex(ptr, end); }

#include <stdarg.h>
int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
int printf(const char *fmt,...)
{
  char buf[1024];
  va_list args;
  int i, j;

  va_start(args, fmt);
  i=vsnprintf(buf,sizeof(buf), fmt, args);
  va_end(args);

  for(j = 0; j < i; j++)
    putchar(buf[j]);

  return i;
}
#define DELAY(n) do { \
  unsigned __n=(n); \
  while(__n--); \
} while(0);
//////////////////////////////////////////////////

void tsk_foo(void *pv)
{
  printf("This is foo\r\n");
  task_exit(0);
}

void main(void *pv)
{
  printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
         task_getid(), pv);

  /*
   * Print all supported graphics modes
   */
//  listGraphicsModes(); 

  /*
   * Initialise the graphics system with specified mode
   *
   * For example, mode 0x115 means 800x600x24, that is,
   *      g_mib.XResolution = 800
   *      g_mib.YResolution = 600
   *      g_mib.BitsPerPixel= 24
   */
//  initGraphics(0x115);   

  if(0) {
    /*
     * Example of creating an EPOS task
     */
    int tid_foo;
    unsigned char *stack_foo;

    stack_foo = (unsigned char *)malloc(1024*1024);
    tid_foo = task_create(stack_foo+1024*1024, tsk_foo, (void *)0);
  }

  while(1)
    ;
  task_exit(0);
}

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
  init_memory_pool(64*1024*1024, end);
}

