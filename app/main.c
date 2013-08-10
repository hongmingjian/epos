
#include <stdarg.h>

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int putchar(int c);
int task_getid();
void task_yield();
int task_sleep(unsigned msec);
int task_create(unsigned stack, void *func, unsigned pv);
int task_exit(int val);

int printf(const char *fmt,...)
{
	char buf[1024];
	va_list args;
	int i, j;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	for(j = 0; j < i; j++)
		putchar(buf[j]);

	return i;
}

static unsigned fib(unsigned n)
{
     if (n == 0)
        return 0;
     if (n == 1)
        return 1;
     return fib(n - 1) + fib(n - 2);
}

static void foo(void *pv)
{
    int i;
    printf("%d: pv=0x%08x\n\r", task_getid(), pv);

    for(i = 38; i < 48; i++)
        printf("%d: fib(%d)=%u\n\r", task_getid(), i, fib(i));
    
    printf("%d: Exiting\n\r", task_getid());

    task_exit((int)pv);
}

void main(void *pv)
{
  printf("%d: Hello, I'm the first user task!\n\r", task_getid());
  printf("%d: pv=0x%08x\n\r", task_getid(), pv);

  printf("%d: task #%d created\n\r", task_getid(), task_create(0x10000000, foo, 0x19760206));

  while(1) {
    task_sleep(500000);
    printf("%d: fib(%d)=%u\n\r", task_getid(), 37, fib(37));
  }
}

void __main()
{
}

