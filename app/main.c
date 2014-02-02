#include "../global.h"

/////////////////SYSTEM CALLS/////////////////////
int task_exit(int val);
int task_create(unsigned stack, void *func, unsigned pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *exit_code);
void beep(unsigned freq);
int putchar(int c);

struct vm86_context {
  uint32_t  edi;       /* 0*/
  uint32_t  esi;       /* 4*/
  uint32_t  ebp;       /* 8*/
  uint32_t  isp;       /*12*/
  uint32_t  ebx;       /*16*/
  uint32_t  edx;       /*20*/
  uint32_t  ecx;       /*24*/
  uint32_t  eax;       /*28*/
  /* below defined in x86 hardware */
  uint32_t  eip;       /*32*/
  uint32_t   cs;       /*36*/
  uint32_t  eflags;    /*40*/
  uint32_t  esp;       /*44*/
  uint32_t   ss;       /*48*/
  uint32_t   es;       /*52*/
  uint32_t   ds;       /*56*/
  uint32_t   fs;       /*60*/
  uint32_t   gs;       /*64*/
};
int vm86(struct vm86_context *v86c);

///////////////////HELPERS///////////////////////
void srand(uint32_t x);
uint32_t random();

#include "../tlsf/tlsf.h"
extern char end[];
void *malloc(size_t bytes)
{ return malloc_ex(bytes, end); }
void *realloc(void *oldptr, size_t bytes)
{ return realloc_ex(oldptr, bytes, end); }
void free(void *ptr)
{ free_ex(ptr, end); }

#include <stdarg.h>
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
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

//////////////////////////////////////////////////
#define DELAY(n) do { \
  unsigned __n=(n); \
  while(__n--); \
} while(0);

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

void main(void *pv)
{

  printf("task #%d: Hello world! I'm the first user task(pv=0x%08x)!\r\n",
         task_getid(), pv);

  if(0) {
    uint16_t *p;
    struct vm86_context v86c;
    memset(&v86c, 0, sizeof(v86c));
    p = (uint16_t *)(0x12*4+0);
    v86c.eip = *p;
    p = (uint16_t *)(0x12*4+2);
    v86c.cs = *p;
    v86c.eflags = 0;
    v86c.esp = 0;
    v86c.ss = 0x1000;

    printf("vm86() returns: %d\r\n", vm86(&v86c));
  }

  if(1){
    int code;
    int tid_hanoi, tid_fib;
    char *stack_hanoi, *stack_fib;

    stack_hanoi = malloc(1024*1024);
    tid_hanoi = task_create(stack_hanoi+1024*1024, tsk_hanoi, (void *)6);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_hanoi, stack_hanoi, 1024*1024);

    stack_fib = malloc(1024*1024);
    tid_fib = task_create(stack_fib+1024*1024, tsk_fib, (void *)tid_hanoi);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_fib,   stack_fib,   1024*1024);

    printf("task #%d: waiting task #%d to exit\r\n", task_getid(), tid_hanoi);
    task_wait(tid_hanoi, &code);
    free(stack_hanoi);
    printf("task #%d: task #%d exited with code %d\r\n",
           task_getid(), tid_hanoi, code);
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

