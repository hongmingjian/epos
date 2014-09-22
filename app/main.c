#include "../global.h"
#include "syscall.h"
#include "math.h"

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
	i=vsnprintf(buf,sizeof(buf), fmt,args);
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

static void hanoi(int d, char from, char to, char aux)
{
  unsigned n;

  if(d == 1) {
    DELAY(1000000);
    printf("%c%d%c ", from, d, to);
    return;
  }

  hanoi(d - 1, from, aux, to);
  DELAY(1000000);
  printf("%c%d%c ", from, d, to);
  hanoi(d - 1, aux, to, from);
}

static tsk_hanoi(void *pv)
{
  int n = (int)pv;
  printf("task #%d: Playing Hanoi tower with %d plates\r\n",
         task_getid(), pv);
  if(n <= 0) {
    printf("task #%d: Illegal number of plates %d\r\n", n);
    task_exit(-1);
  }

  hanoi(n, 'A', 'B', 'C');

  printf("task #%d: Exiting\r\n", task_getid());
  task_exit(n);
}

static unsigned fib(unsigned n)
{
  if (n == 0)
    return 0;
  if (n == 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

static void tsk_fib(void *pv)
{
  int i, code;

  printf("task #%d: waiting task #%d to exit\r\n", task_getid(), (int)pv);
  task_wait((int)pv, &code);
  printf("task #%d: task #%d exited with code %d\r\n",
         task_getid(), (int)pv, code);

  for(i = 37; i < 48; i++)
    printf("task #%d: fib(%d)=%u\r\n", task_getid(), i, fib(i));

  printf("task #%d: Exiting\r\n", task_getid());

  task_exit(0);
}

void testGraphics()
{
  if(initGraphics(0x115)) {
    return;
  }

  drawMoire();
  DELAY(200000000);

  drawCheckerboard();
  DELAY(200000000);

  mandelbrot();
  DELAY(200000000);

  exitGraphics();

  listGraphicsModes();
}

void main(void *pv)
{
  printf("task #%d: Hello world! I'm the first user task(pv=0x%08x)!\r\n",
         task_getid(), pv);

  testGraphics();

  if(1){
    int code;
    int tid_hanoi, tid_fib;
    char *stack_hanoi, *stack_fib;

    stack_hanoi = malloc(1024*1024);
    tid_hanoi = task_create((unsigned)(stack_hanoi+1024*1024), tsk_hanoi, 6);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_hanoi, stack_hanoi, 1024*1024);

    stack_fib = malloc(1024*1024);
    tid_fib = task_create((unsigned)(stack_fib+1024*1024), tsk_fib, tid_hanoi);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_fib,   stack_fib,   1024*1024);

    printf("task #%d: waiting task #%d to exit\r\n", task_getid(), tid_hanoi);
    task_wait(tid_hanoi, &code);
    free(stack_hanoi);
    printf("task #%d: task #%d exited with code %d\r\n",
           task_getid(), tid_hanoi, code);
  }

  if (0) {
    printf("task #%d: sin(%f) = %f\r\n", task_getid(), M_PI/6, sin(M_PI/6));
    printf("task #%d: sqrt(%f) = %f\r\n", task_getid(), 2.0, sqrt(2.0));
    printf("task #%d: pow(%f, %f) = %f\r\n", task_getid(), 2.0, 1.0/2.0, pow(2.0, 1.0/2.0));
    printf("task #%d: log2(%f) = %f\r\n", task_getid(), 4.0, log2(4.0, 1.0));
    printf("task #%d: atan2(%f, %f) = %f\r\n", task_getid(), 10.0, -10.0, atan2(10.0, -10.0)*180/M_PI);
    printf("task #%d: exp(%f) = %f\r\n", task_getid(), 5.0, exp(5.0));
    printf("task #%d: tan(%f) = %f\r\n", task_getid(), 30.0, tan(30.0*M_PI/180));
    printf("task #%d: cot(%f) = %f\r\n", task_getid(), 30.0, cot(30.0*M_PI/180));
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

