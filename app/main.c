
#include <stdarg.h>

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
void putc(int c);

int printf(const char *fmt,...)
{
	char buf[1024];
	va_list args;
	int i, j;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	for(j = 0; j < i; j++)
		putc(buf[j]);

	return i;
}

void main(void *pv)
{
  printf("Hello, world!\n\r");
  printf("pv=0x%08x\n\r", pv);

//  *(int *)0 = 0;

  while(1) {
    int i;
    for(i = 0; i < 1000000; i++)
      ;
    printf("U");
  }
}

void __main()
{
}
