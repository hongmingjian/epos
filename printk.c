#include <stdarg.h>
#include "utils.h"

int printk(const char *fmt,...)
{
  char buf[256];
  va_list args;
  int i, j;

  va_start(args, fmt);
  i=vsprintf(buf,fmt,args);
  va_end(args);

  for(j = 0; j < i; j++)
    sys_putchar(buf[j]);

  return i;
}

